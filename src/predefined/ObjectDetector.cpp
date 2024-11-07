/**
* This file contains object detector pipeline and interface for Unity scene called "Object Detector"
* Main goal is to perform object detection + depth
*/

#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

// ------------------------------------------------------------------------
// Plugin itself

#include <iostream>
#include <cstdio>
#include <random>

#include "../utility.hpp"

// Common necessary includes for development using depthai library
#include "depthai/depthai.hpp"
#include "depthai/device/Device.hpp"

#include "depthai-unity/predefined/ObjectDetector.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"


static const std::vector<std::string> labelMap = {
    "person",        "bicycle",      "car",           "motorbike",     "aeroplane",   "bus",         "train",       "truck",        "boat",
    "traffic light", "fire hydrant", "stop sign",     "parking meter", "bench",       "bird",        "cat",         "dog",          "horse",
    "sheep",         "cow",          "elephant",      "bear",          "zebra",       "giraffe",     "backpack",    "umbrella",     "handbag",
    "tie",           "suitcase",     "frisbee",       "skis",          "snowboard",   "sports ball", "kite",        "baseball bat", "baseball glove",
    "skateboard",    "surfboard",    "tennis racket", "bottle",        "wine glass",  "cup",         "fork",        "knife",        "spoon",
    "bowl",          "banana",       "apple",         "sandwich",      "orange",      "broccoli",    "carrot",      "hot dog",      "pizza",
    "donut",         "cake",         "chair",         "sofa",          "pottedplant", "bed",         "diningtable", "toilet",       "tvmonitor",
    "laptop",        "mouse",        "remote",        "keyboard",      "cell phone",  "microwave",   "oven",        "toaster",      "sink",
    "refrigerator",  "book",         "clock",         "vase",          "scissors",    "teddy bear",  "hair drier",  "toothbrush"};

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration 
* @returns pipeline 
*/
dai::Pipeline createObjectDetectorPipeline(PipelineConfig *config)
{
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::XLinkOut> xlinkOut;

    auto spatialDetectionNetwork = pipeline.create<dai::node::YoloSpatialDetectionNetwork>();

    auto colorCam = pipeline.create<dai::node::ColorCamera>();

    // Color camera preview
    if (config->previewSizeWidth > 0 && config->previewSizeHeight > 0) 
    {
        xlinkOut = pipeline.create<dai::node::XLinkOut>();
        xlinkOut->setStreamName("preview");
        colorCam->setPreviewSize(config->previewSizeWidth, config->previewSizeHeight);
    }

    // Color camera properties            
    colorCam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    if (config->colorCameraResolution == 1) colorCam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_4_K);
    if (config->colorCameraResolution == 2) colorCam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_12_MP);
    if (config->colorCameraResolution == 3) colorCam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_13_MP);
    colorCam->setInterleaved(config->colorCameraInterleaved);
    colorCam->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);
    if (config->colorCameraColorOrder == 1) colorCam->setColorOrder(dai::ColorCameraProperties::ColorOrder::RGB);
    colorCam->setFps(config->colorCameraFPS);

    // NN
    spatialDetectionNetwork->setBlobPath(config->nnPath1);
    spatialDetectionNetwork->setConfidenceThreshold(0.5f);
    spatialDetectionNetwork->input.setBlocking(false);
    spatialDetectionNetwork->setBoundingBoxScaleFactor(0.5);
    spatialDetectionNetwork->setDepthLowerThreshold(100);
    spatialDetectionNetwork->setDepthUpperThreshold(5000);

    // yolo specific parameters
    spatialDetectionNetwork->setNumClasses(80);
    spatialDetectionNetwork->setCoordinateSize(4);
    spatialDetectionNetwork->setAnchors({10, 14, 23, 27, 37, 58, 81, 82, 135, 169, 344, 319});
    spatialDetectionNetwork->setAnchorMasks({{"side26", {1, 2, 3}}, {"side13", {3, 4, 5}}});
    spatialDetectionNetwork->setIouThreshold(0.5f);

    colorCam->preview.link(spatialDetectionNetwork->input);
    spatialDetectionNetwork->passthrough.link(xlinkOut->input);

    // output of neural network
    auto nnOut = pipeline.create<dai::node::XLinkOut>();
    nnOut->setStreamName("detections");    

    // Depth
    if (config->confidenceThreshold > 0)
    {
        auto left = pipeline.create<dai::node::MonoCamera>();
        auto right = pipeline.create<dai::node::MonoCamera>();
        auto stereo = pipeline.create<dai::node::StereoDepth>();

        // For RGB-Depth align
        if (config->ispScaleF1 > 0 && config->ispScaleF2 > 0) colorCam->setIspScale(config->ispScaleF1, config->ispScaleF2);
        if (config->manualFocus > 0) colorCam->initialControl.setManualFocus(config->manualFocus);

        // Mono camera properties    
        left->setResolution(dai::MonoCameraProperties::SensorResolution::THE_400_P);
        if (config->monoLCameraResolution == 1) left->setResolution(dai::MonoCameraProperties::SensorResolution::THE_720_P);
        if (config->monoLCameraResolution == 2) left->setResolution(dai::MonoCameraProperties::SensorResolution::THE_800_P);
        if (config->monoLCameraResolution == 3) left->setResolution(dai::MonoCameraProperties::SensorResolution::THE_480_P);
        left->setBoardSocket(dai::CameraBoardSocket::LEFT);
        right->setResolution(dai::MonoCameraProperties::SensorResolution::THE_400_P);
        if (config->monoRCameraResolution == 1) right->setResolution(dai::MonoCameraProperties::SensorResolution::THE_720_P);
        if (config->monoRCameraResolution == 2) right->setResolution(dai::MonoCameraProperties::SensorResolution::THE_800_P);
        if (config->monoRCameraResolution == 3) right->setResolution(dai::MonoCameraProperties::SensorResolution::THE_480_P);
        right->setBoardSocket(dai::CameraBoardSocket::RIGHT);

        // Stereo properties
        stereo->setConfidenceThreshold(config->confidenceThreshold);
        // LR-check is required for depth alignment
        stereo->setLeftRightCheck(config->leftRightCheck);
        if (config->depthAlign > 0) stereo->setDepthAlign(dai::CameraBoardSocket::RGB);
        stereo->setSubpixel(config->subpixel);
        
        stereo->initialConfig.setMedianFilter(dai::MedianFilter::MEDIAN_OFF);
        if (config->medianFilter == 1) stereo->initialConfig.setMedianFilter(dai::MedianFilter::KERNEL_3x3);
        if (config->medianFilter == 2) stereo->initialConfig.setMedianFilter(dai::MedianFilter::KERNEL_5x5);
        if (config->medianFilter == 3) stereo->initialConfig.setMedianFilter(dai::MedianFilter::KERNEL_7x7);

        // Linking
        left->out.link(stereo->left);
        right->out.link(stereo->right);
        auto xoutDepth = pipeline.create<dai::node::XLinkOut>();            
        xoutDepth->setStreamName("depth");
        stereo->depth.link(xoutDepth->input);

        
        auto xoutBoundingBoxDepthMapping = pipeline.create<dai::node::XLinkOut>();
        xoutBoundingBoxDepthMapping->setStreamName("boundingBoxDepthMapping");

        spatialDetectionNetwork->out.link(nnOut->input);
        spatialDetectionNetwork->boundingBoxMapping.link(xoutBoundingBoxDepthMapping->input);

        stereo->depth.link(spatialDetectionNetwork->inputDepth);
        spatialDetectionNetwork->passthroughDepth.link(xoutDepth->input);

    }

    // SYSTEM INFORMATION
    if (config->rate > 0.0f)
    {
        // Define source and output
        auto sysLog = pipeline.create<dai::node::SystemLogger>();
        auto xout = pipeline.create<dai::node::XLinkOut>();

        xout->setStreamName("sysinfo");

        // Properties
        sysLog->setRate(config->rate);  // 1 hz updates

        // Linking
        sysLog->out.link(xout->input);
    }

    // IMU
    if (config->freq > 0)
    {
        auto imu = pipeline.create<dai::node::IMU>();
        auto xlinkOutImu = pipeline.create<dai::node::XLinkOut>();

        xlinkOutImu->setStreamName("imu");

        // enable ROTATION_VECTOR at 400 hz rate
        imu->enableIMUSensor(dai::IMUSensor::ROTATION_VECTOR, config->freq);
        // above this threshold packets will be sent in batch of X, if the host is not blocked and USB bandwidth is available
        imu->setBatchReportThreshold(config->batchReportThreshold);
        // maximum number of IMU packets in a batch, if it's reached device will block sending until host can receive it
        // if lower or equal to batchReportThreshold then the sending is always blocking on device
        // useful to reduce device's CPU load  and number of lost packets, if CPU load is high on device side due to multiple nodes
        imu->setMaxBatchReports(config->maxBatchReports);

        // Link plugins IMU -> XLINK
        imu->out.link(xlinkOutImu->input);
    }

    return pipeline;
}

extern "C"
{
   /**
    * Pipeline creation based on streams template
    *
    * @param config pipeline configuration 
    * @returns pipeline 
    */
    EXPORT_API bool InitObjectDetector(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createObjectDetectorPipeline(config);

        // If deviceId is empty .. just pick first available device
        bool res = false;

        if (strcmp(config->deviceId,"NONE")==0 || strcmp(config->deviceId,"")==0) res = DAIStartPipeline(pipeline,config->deviceNum,NULL);        
        else res = DAIStartPipeline(pipeline,config->deviceNum,config->deviceId);
        
        return res;
    }

    /**
    * Pipeline results
    *
    * @param frameInfo camera images pointers
    * @param getPreview True if color preview image is requested, False otherwise. Requires previewSize in pipeline creation.
    * @param useDepth True if depth information is requested, False otherwise. Requires confidenceThreshold in pipeline creation.
    * @param retrieveInformation True if system information is requested, False otherwise. Requires rate in pipeline creation.
    * @param useIMU True if IMU information is requested, False otherwise. Requires freq in pipeline creation.
    * @param deviceNum Device selection on unity dropdown
    * @returns Json with results or information about device availability. 
    */    

    /**
    * Example of json returned
    * { "objects": [ {"label":"object","score":0.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,"ymax":0.0,"xcenter":0.0,"ycenter":0.0},{"label":1,"score":1.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,* "ymax":0.0,"xcenter":0.0,"ycenter":0.0}],"best":{"label":1,"score":1.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,"ymax":0.0,"xcenter":0.0,"ycenter":0.0},"fps":0.0}
    */

    EXPORT_API const char* ObjectDetectorResults(FrameInfo *frameInfo, bool getPreview, float objectScoreThreshold, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum)
    {
        using namespace std;
        using namespace std::chrono;

        // Get device deviceNum
        std::shared_ptr<dai::Device> device = GetDevice(deviceNum);
        // Device no available
        if (device == NULL) 
        {
            char* ret = (char*)::malloc(strlen("{\"error\":\"NO_DEVICE\"}"));
            ::memcpy(ret, "{\"error\":\"NO_DEVICE\"}",strlen("{\"error\":\"NO_DEVICE\"}"));
            ret[strlen("{\"error\":\"NO_DEVICE\"}")] = 0;
            return ret;
        }

        // If device deviceNum is running pipeline
        if (IsDeviceRunning(deviceNum))
        {
            // object info
            nlohmann::json objectDetectorJson = {};

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;
            
            // object detector results
            auto detectionNNQueue = device->getOutputQueue("detections",4,false);
            
            // if preview image is requested. True in this case.
            if (getPreview) preview = device->getOutputQueue("preview",4,false);
            
            // if depth images are requested. All images.
            depthQueue = device->getOutputQueue("depth", 4, false);
            
            auto xoutBoundingBoxDepthMappingQueue = device->getOutputQueue("boundingBoxDepthMapping", 4, false);

            int countd;
            auto color = cv::Scalar(255, 255, 255);

            auto imgFrame = preview->get<dai::ImgFrame>();
            auto inDet = detectionNNQueue->get<dai::SpatialImgDetections>();
            auto depth = depthQueue->get<dai::ImgFrame>();

            cv::Mat frame = imgFrame->getCvFrame();
            cv::Mat depthFrame = depth->getFrame();

            int count;
            // In this case we allocate before Texture2D (ARGB32) and memcpy pointer data 
            
            auto detections = inDet->detections;
            if(!detections.empty()) {
                
                auto boundingBoxMapping = xoutBoundingBoxDepthMappingQueue->get<dai::SpatialLocationCalculatorConfig>();
                auto roiDatas = boundingBoxMapping->getConfigData();

                for(auto roiData : roiDatas) {
                    auto roi = roiData.roi;
                    roi = roi.denormalize(depthFrame.cols, depthFrame.rows);
                    auto topLeft = roi.topLeft();
                    auto bottomRight = roi.bottomRight();
                    auto xmin = (int)topLeft.x;
                    auto ymin = (int)topLeft.y;
                    auto xmax = (int)bottomRight.x;
                    auto ymax = (int)bottomRight.y;

                    cv::rectangle(depthFrame, cv::Rect(cv::Point(xmin, ymin), cv::Point(xmax, ymax)), color, cv::FONT_HERSHEY_SIMPLEX);
                }
            }

            nlohmann::json objectsArr = {};

            for(const auto& detection : detections) {

                int x1 = detection.xmin * frame.cols;
                int y1 = detection.ymin * frame.rows;
                int x2 = detection.xmax * frame.cols;
                int y2 = detection.ymax * frame.rows;

                int labelIndex = detection.label;
                std::string labelStr = to_string(labelIndex);
                if(labelIndex < labelMap.size()) {
                    labelStr = labelMap[labelIndex];
                }

                if (detection.confidence>=objectScoreThreshold) 
                {
                    cv::rectangle(frame, cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)), color, cv::FONT_HERSHEY_SIMPLEX);
                
                    nlohmann::json object;
                    object["label"] = labelStr;
                    object["score"] = detection.confidence * 100;
                    object["xmin"] = x1; 
                    object["xmax"] = x2; 
                    object["ymin"] = y1; 
                    object["ymax"] = y2; 
                    object["X"] = (int)detection.spatialCoordinates.x;
                    object["Y"] = (int)detection.spatialCoordinates.y;
                    object["Z"] = (int)detection.spatialCoordinates.z;

                    objectsArr.push_back(object);
                }
            }

            toARGB(frame,frameInfo->colorPreviewData);

            objectDetectorJson["objects"] = objectsArr;

            // SYSTEM INFORMATION
            if (retrieveInformation) objectDetectorJson["sysinfo"] = GetDeviceInfo(device);        
            // IMU
            if (useIMU) objectDetectorJson["imu"] = GetIMU(device);

            char* ret = (char*)::malloc(strlen(objectDetectorJson.dump().c_str())+1);
            ::memcpy(ret, objectDetectorJson.dump().c_str(),strlen(objectDetectorJson.dump().c_str()));
            ret[strlen(objectDetectorJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }


}