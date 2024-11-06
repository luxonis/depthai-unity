/**
* This file contains face detector pipeline and interface for Unity scene called "Face Detector"
* Main goal is to perform face detection + depth
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

#include "depthai-unity/predefined/FaceDetector.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"


dai::SpatialLocationCalculatorConfigData sconfig;
dai::SpatialLocationCalculatorAlgorithm calculationAlgorithm;

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration
* @returns pipeline
*/
dai::Pipeline createFaceDetectorPipeline(PipelineConfig *config)
{
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::XLinkOut> xlinkOut;

    auto colorCam = pipeline.create<dai::node::ColorCamera>();

    // Color camera preview
    if (config->previewSizeWidth > 0 && config->previewSizeHeight > 0)
    {
        xlinkOut = pipeline.create<dai::node::XLinkOut>();
        xlinkOut->setStreamName("preview");

        // stretch
        //colorCam->setPreviewKeepAspectRatio(false);

        // normal crop <- @todo: add parameter in unity
        // not for letterbox <- compute on Unity
        //colorCam->setPreviewSize(config->previewSizeWidth, config->previewSizeHeight);
        //colorCam->preview.link(xlinkOut->input);

        // letterbox
        // compute resolution with ipscale
        int resx = 1920;
        int resy = 1080;
        if (config->colorCameraResolution == 1)
        {
            resx = 3840;
            resy = 2160;
        }
        if (config->colorCameraResolution == 2)
        {
            resx = 4056;
            resy = 3040;
        }
        if (config->colorCameraResolution == 3)
        {
            resx = 4208;
            resy = 3120;
        }

        if (config->ispScaleF1 > 0 && config->ispScaleF2 > 0)
        {
            resx = resx * ((float)config->ispScaleF1/(float)config->ispScaleF2);
            resy = resy * ((float)config->ispScaleF1/(float)config->ispScaleF2);
        }
        colorCam->setPreviewSize(resx,resy);
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


    // letterbox
    auto manip1 = pipeline.create<dai::node::ImageManip>();
    manip1->initialConfig.setResizeThumbnail(300,300);
    colorCam->preview.link(manip1->inputImage);

    // neural network
    auto nn1 = pipeline.create<dai::node::NeuralNetwork>();
    nn1->setBlobPath(config->nnPath1);

    // not for letterbox
    manip1->out.link(nn1->input);
    manip1->out.link(xlinkOut->input);
    //colorCam->preview.link(nn1->input);

    // output of neural network
    auto nnOut = pipeline.create<dai::node::XLinkOut>();
    nnOut->setStreamName("detections");
    nn1->out.link(nnOut->input);

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

        stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_DENSITY);

        // Spatial Locator
        auto spatialDataCalculator = pipeline.create<dai::node::SpatialLocationCalculator>();
        auto xoutSpatialData = pipeline.create<dai::node::XLinkOut>();
        auto xinSpatialCalcConfig = pipeline.create<dai::node::XLinkIn>();

        xoutSpatialData->setStreamName("spatialData");
        xinSpatialCalcConfig->setStreamName("spatialCalcConfig");


        dai::Point2f topLeft(0.4f, 0.4f);
        dai::Point2f bottomRight(0.6f, 0.6f);

        sconfig.depthThresholds.lowerThreshold = 100;
        sconfig.depthThresholds.upperThreshold = 10000;
        auto calculationAlgorithm = dai::SpatialLocationCalculatorAlgorithm::MEDIAN;
        sconfig.calculationAlgorithm = calculationAlgorithm;
        sconfig.roi = dai::Rect(topLeft, bottomRight);

        spatialDataCalculator->inputConfig.setWaitForMessage(false);

        // Linking
        left->out.link(stereo->left);
        right->out.link(stereo->right);
        auto xoutDepth = pipeline.create<dai::node::XLinkOut>();
        xoutDepth->setStreamName("depth");
        stereo->depth.link(xoutDepth->input);


        spatialDataCalculator->passthroughDepth.link(xoutDepth->input);
        stereo->depth.link(spatialDataCalculator->inputDepth);

        spatialDataCalculator->out.link(xoutSpatialData->input);
        xinSpatialCalcConfig->out.link(spatialDataCalculator->inputConfig);

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
    EXPORT_API bool InitFaceDetector(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createFaceDetectorPipeline(config);

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
    * { "faces": [ {"label":0,"score":0.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,"ymax":0.0,"xcenter":0.0,"ycenter":0.0},{"label":1,"score":1.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,* "ymax":0.0,"xcenter":0.0,"ycenter":0.0}],"best":{"label":1,"score":1.0,"xmin":0.0,"ymin":0.0,"xmax":0.0,"ymax":0.0,"xcenter":0.0,"ycenter":0.0},"fps":0.0}
    */

    EXPORT_API const char* FaceDetectorResults(FrameInfo *frameInfo, bool getPreview, bool drawBestFaceInPreview, bool drawAllFacesInPreview, float faceScoreThreshold, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum)
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
            // preview image
            cv::Mat frame;
            std::shared_ptr<dai::ImgFrame> imgFrame;

            // other images
            cv::Mat depthFrame, depthFrameOrig, dispFrameOrig, dispFrame, monoRFrameOrig, monoRFrame, monoLFrameOrig, monoLFrame;

            // face info
            nlohmann::json faceDetectorJson = {};

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;
            std::shared_ptr<dai::DataOutputQueue> spatialCalcQueue;
            std::shared_ptr<dai::DataInputQueue> spatialCalcConfigInQueue;

            // face detector results
            auto detections = device->getOutputQueue("detections",1,false);

            // if preview image is requested. True in this case.
            if (getPreview) preview = device->getOutputQueue("preview",1,false);

            // if depth images are requested. All images.
            if (useDepth)
            {
                depthQueue = device->getOutputQueue("depth", 8, false);
                spatialCalcQueue = device->getOutputQueue("spatialData", 8, false);
                spatialCalcConfigInQueue = device->getInputQueue("spatialCalcConfig");
            }

            int countd;

            if (getPreview)
            {
                auto imgFrames = preview->tryGetAll<dai::ImgFrame>();
                countd = imgFrames.size();
                if (countd > 0) {
                    auto imgFrame = imgFrames[countd-1];
                    if(imgFrame){
                        frame = toMat(imgFrame->getData(), imgFrame->getWidth(), imgFrame->getHeight(), 3, 1);
                    }
                }
            }

            vector<std::shared_ptr<dai::ImgFrame>> imgDepthFrames,imgDispFrames,imgMonoRFrames,imgMonoLFrames;
            std::shared_ptr<dai::ImgFrame> imgDepthFrame,imgDispFrame,imgMonoRFrame,imgMonoLFrame;

            int count;
            // In this case we allocate before Texture2D (ARGB32) and memcpy pointer data
            if (useDepth)
            {
                // Depth
                imgDepthFrames = depthQueue->tryGetAll<dai::ImgFrame>();
                count = imgDepthFrames.size();
                if (count > 0)
                {
                    imgDepthFrame = imgDepthFrames[count-1];
                    depthFrameOrig = imgDepthFrame->getFrame();
                    cv::normalize(depthFrameOrig, depthFrame, 255, 0, cv::NORM_INF, CV_8UC1);
                    cv::equalizeHist(depthFrame, depthFrame);
                    cv::cvtColor(depthFrame, depthFrame, cv::COLOR_GRAY2BGR);
                }
            }

            // Face detection results
            struct Detection {
                unsigned int label;
                float score;
                float x_min;
                float y_min;
                float x_max;
                float y_max;
            };

            vector<Detection> dets;

            auto det = detections->get<dai::NNData>();
            std::vector<float> detData = det->getFirstLayerFp16();
            float maxScore = 0.0;
            int maxPos = 0;

            nlohmann::json facesArr = {};
            nlohmann::json bestFace = {};

            dai::SpatialLocationCalculatorConfig cfg;

            if(detData.size() > 0){
                int i = 0;
                while (detData[i*7] != -1.0f && i*7 < (int)detData.size()) {

                    Detection d;
                    d.label = detData[i*7 + 1];
                    d.score = detData[i*7 + 2];
                    if (d.score > maxScore)
                    {
                        maxScore = d.score;
                        maxPos = i;
                    }
                    d.x_min = detData[i*7 + 3];
                    d.y_min = detData[i*7 + 4];
                    d.x_max = detData[i*7 + 5];
                    d.y_max = detData[i*7 + 6];
                    i++;

                    if (faceScoreThreshold <= d.score)
                    {
                        int x1 = d.x_min * frame.cols;
                        int y1 = d.y_min * frame.rows;
                        int x2 = d.x_max * frame.cols;
                        int y2 = d.y_max * frame.rows;
                        int mx = x1 + ((x2 - x1) / 2);
                        int my = y1 + ((y2 - y1) / 2);

                        //sconfig.roi = prepareComputeDepth(depthFrame,frame,mx,my,0);
                        sconfig.roi = prepareComputeDepth(depthFrame,frame,mx,my,1);
                        sconfig.calculationAlgorithm = calculationAlgorithm;
                        cfg.addROI(sconfig);

                        dets.push_back(d);
                    }
                }
            }


            // send spatial
            if (dets.size() > 0)
            {

                if (useDepth) spatialCalcConfigInQueue->send(cfg);

                // get spatial
                std::vector<dai::SpatialLocations> spatialData;
                if (useDepth) spatialData = spatialCalcQueue->get<dai::SpatialLocationCalculatorData>()->getSpatialLocations();

                int i = 0;
                // write jsons
                for(auto d : dets) {
                    nlohmann::json face;
                    face["label"] = d.label;
                    face["score"] = d.score;
                    face["xmin"] = d.x_min;
                    face["ymin"] = d.y_min;
                    face["xmax"] = d.x_max;
                    face["ymax"] = d.y_max;
                    int x1 = d.x_min * frame.cols;
                    int y1 = d.y_min * frame.rows;
                    int x2 = d.x_max * frame.cols;
                    int y2 = d.y_max * frame.rows;
                    int mx = x1 + ((x2 - x1) / 2);
                    int my = y1 + ((y2 - y1) / 2);
                    face["xcenter"] = mx;
                    face["ycenter"] = my;

                    if (getPreview && countd > 0 && drawAllFacesInPreview) cv::rectangle(frame, cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)), cv::Scalar(255,255,255));

                    if (useDepth) {
                        auto roi = spatialData[i].config.roi;
                        roi = roi.denormalize(depthFrame.cols, depthFrame.rows);

                        face["X"] = (int)spatialData.at(i).spatialCoordinates.x;
                        face["Y"] = (int)spatialData.at(i).spatialCoordinates.y;
                        face["Z"] = (int)spatialData.at(i).spatialCoordinates.z;
                    }
                    facesArr.push_back(face);

                    if (i == maxPos)
                    {
                        bestFace["label"] = d.label;
                        bestFace["score"] = d.score;
                        bestFace["xmin"] = d.x_min;
                        bestFace["ymin"] = d.y_min;
                        bestFace["xmax"] = d.x_max;
                        bestFace["ymax"] = d.y_max;
                        bestFace["xcenter"] = mx;
                        bestFace["ycenter"] = my;

                        if (useDepth) {
                            bestFace["X"] = (int)spatialData.at(i).spatialCoordinates.x;
                            bestFace["Y"] = (int)spatialData.at(i).spatialCoordinates.y;
                            bestFace["Z"] = (int)spatialData.at(i).spatialCoordinates.z;
                        }

                        if (getPreview && countd > 0 && drawBestFaceInPreview)
                        {
                            cv::rectangle(frame, cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)), cv::Scalar(255,255,255));
                        }
                    }

                    i++;
                }
            }

            if (getPreview && countd>0) toARGB(frame,frameInfo->colorPreviewData);

            faceDetectorJson["faces"] = facesArr;
            faceDetectorJson["best"] = bestFace;

            // SYSTEM INFORMATION
            if (retrieveInformation) faceDetectorJson["sysinfo"] = GetDeviceInfo(device);
            // IMU
            if (useIMU) faceDetectorJson["imu"] = GetIMU(device);

            char* ret = (char*)::malloc(strlen(faceDetectorJson.dump().c_str())+1);
            ::memcpy(ret, faceDetectorJson.dump().c_str(),strlen(faceDetectorJson.dump().c_str()));
            ret[strlen(faceDetectorJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }


}