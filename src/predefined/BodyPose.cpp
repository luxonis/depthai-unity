/**
* This file contains Body Pose pipeline and interface for Unity scene called "Body Pose"
* Main goal is to perform body pose + depth. It's using MoveNet model.
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

#include "depthai-unity/predefined/BodyPose.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"

cv::Scalar ColorForLandmark(int landm)
{
    cv::Scalar color;

    if (landm % 2 == 1) color = cv::Scalar(0,255,0);
    else if (landm == 0) color = cv::Scalar(0,255,255);
    else color = cv::Scalar(0,0,255);
    
    return color;
}

int pad = 192;
int mframe = 0;

void SetCameraPreviewSize(std::shared_ptr<dai::node::ColorCamera> colorCam, PipelineConfig *config)
{
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

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration 
* @returns pipeline 
*/

dai::Pipeline createBodyPosePipeline(PipelineConfig *config)
{
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::XLinkOut> xlinkOut;
    
    auto colorCam = pipeline.create<dai::node::ColorCamera>();

    // Color camera preview
    if (config->previewSizeWidth > 0 && config->previewSizeHeight > 0) 
    {
        xlinkOut = pipeline.create<dai::node::XLinkOut>();
        xlinkOut->setStreamName("preview");

        // letterbox
        // compute resolution with ipscale
        if (config->previewMode == 1)
        {
            SetCameraPreviewSize(colorCam,config);
        }
        else 
        {
            colorCam->setPreviewSize(config->previewSizeWidth, config->previewSizeHeight);
            colorCam->preview.link(xlinkOut->input);
        }
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

    // neural network
    auto nn1 = pipeline.create<dai::node::NeuralNetwork>();
    nn1->setBlobPath(config->nnPath1);

    // letterbox
    if (config->previewMode == 1)
    {
        auto manip1 = pipeline.create<dai::node::ImageManip>();
        manip1->initialConfig.setResizeThumbnail(192,192);
        colorCam->preview.link(manip1->inputImage);
    
        manip1->out.link(nn1->input);
        //manip1->out.link(xlinkOut->input);
        nn1->passthrough.link(xlinkOut->input);
    }
    else colorCam->preview.link(nn1->input);

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

        // Linking
        left->out.link(stereo->left);
        right->out.link(stereo->right);
        auto xoutDepth = pipeline.create<dai::node::XLinkOut>();            
        xoutDepth->setStreamName("depth");
        stereo->depth.link(xoutDepth->input);
        
        if (config->useSpatialLocator)
        {
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

            spatialDataCalculator->passthroughDepth.link(xoutDepth->input);
            stereo->depth.link(spatialDataCalculator->inputDepth);

            spatialDataCalculator->out.link(xoutSpatialData->input);
            xinSpatialCalcConfig->out.link(spatialDataCalculator->inputConfig);
        }
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
    EXPORT_API bool InitBodyPose(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createBodyPosePipeline(config);
       
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
    * @param width Unity preview width canvas
    * @param height Unity preview height canvas
    * @param useDepth True if depth information is requested, False otherwise. Requires confidenceThreshold in pipeline creation.
    * @param retrieveInformation True if system information is requested, False otherwise. Requires rate in pipeline creation.
    * @param useIMU True if IMU information is requested, False otherwise. Requires freq in pipeline creation.
    * @param deviceNum Device selection on unity dropdown
    * @returns Json with results or information about device availability. 
    */    
    
    EXPORT_API const char* BodyPoseResults(FrameInfo *frameInfo, bool getPreview, int width, int height, bool useDepth, bool drawBodyPoseInPreview, float bodyLandmarkScoreThreshold, bool retrieveInformation, bool useIMU, bool useSpatialLocator, int deviceNum)
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
            cv::Mat frame;
            cv::Mat depthFrame, depthFrameOrig;
            
            std::shared_ptr<dai::ImgFrame> imgFrame;

            auto startTime = steady_clock::now();
            int counter = 0;
            float fps = 0;

            int LINES_BODY[16][2] = {{4,2},{2,0},{0,1},{1,3},
                {10,8},{8,6},{6,5},{5,7},{7,9},
                {6,12},{12,11},{11,5},
                {12,14},{14,16},{11,13},{13,15}};

            //{[{"index":0,"xpos","ypos","location.x":0,"location.y":0,"location.z":0},{"index":1,"location.x":0,"location.y":0,"location.z":0}]}
            nlohmann::json bodyPoseJson;

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;

             if (getPreview) preview = device->getOutputQueue("preview",1,false);
            
            auto detections = device->getOutputQueue("detections",1,false);
            
            if (useDepth) depthQueue = device->getOutputQueue("depth", 1, false);
            
            if (getPreview)
            {
                auto imgFrames = preview->tryGetAll<dai::ImgFrame>();
                auto countd = imgFrames.size();
                if (countd > 0) {
                    auto imgFrame = imgFrames[countd-1];
                    if(imgFrame){
                        frame = imgFrame->getCvFrame();
                    }
                }
            }
        
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
            std::vector<float> detData = det->getLayerFp16("Identity");
            
            int landmarks_y[17]; 
            int landmarks_x[17];
            int landmarks_xpos[17];
            int landmarks_ypos[17];
            int landmarks_zpos[17];
            float scores[17];

            int count;
            vector<std::shared_ptr<dai::ImgFrame>> imgDepthFrames;
            std::shared_ptr<dai::ImgFrame> imgDepthFrame;
            std::shared_ptr<dai::DataOutputQueue> spatialCalcQueue;
            std::shared_ptr<dai::DataInputQueue> spatialCalcConfigInQueue;

            if (useDepth)
            {            
                if (useSpatialLocator)
                {
                    spatialCalcQueue = device->getOutputQueue("spatialData", 1, false);
                    spatialCalcConfigInQueue = device->getInputQueue("spatialCalcConfig");
                }

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

            nlohmann::json bodyPose = {};
            dai::SpatialLocationCalculatorConfig cfg;

            if(detData.size() > 0){
                int pos = 0;

                int frameSize = pad;
                
                for (int i=0; i<(int)detData.size(); i+=3)
                {
                    landmarks_y[pos] = (int) (detData[i] * frameSize);//frame.rows);
                    landmarks_x[pos] = (int) (detData[i+1] * frameSize);//frame.cols);
                    scores[pos] = detData[i+2];

                    if (useSpatialLocator)
                    {
                        sconfig.roi = prepareComputeDepth(depthFrame,frame,landmarks_x[pos],landmarks_y[pos],1);
                        sconfig.calculationAlgorithm = calculationAlgorithm;
                        cfg.addROI(sconfig);
                    }

                    pos++;
                }

                std::vector<int> pushed;

                
                if (useDepth && pos>0 && useSpatialLocator) 
                {
                    spatialCalcConfigInQueue->send(cfg);
                
                    // get spatial
                    auto spatialData = spatialCalcQueue->get<dai::SpatialLocationCalculatorData>()->getSpatialLocations();
                
                    int i = 0;
                    for(auto depthData : spatialData) {
                        landmarks_xpos[i] = (int)depthData.spatialCoordinates.x;
                        landmarks_ypos[i] = (int)depthData.spatialCoordinates.y;
                        landmarks_zpos[i] = (int)depthData.spatialCoordinates.z;
                        i++;
                    }
                }

                for (int i=0; i<16; i++)
                {
                    if (scores[LINES_BODY[i][0]] > bodyLandmarkScoreThreshold && scores[LINES_BODY[i][1]] > bodyLandmarkScoreThreshold)
                    {
                        if (drawBodyPoseInPreview)
                        {
                            cv::Point point1 = cv::Point(landmarks_x[LINES_BODY[i][0]],landmarks_y[LINES_BODY[i][0]]);
                            cv::Point point2 = cv::Point(landmarks_x[LINES_BODY[i][1]],landmarks_y[LINES_BODY[i][1]]);
                            cv::line(frame, point1, point2 ,cv::Scalar(255, 180, 90));
                            cv::circle(frame, point1, 4, ColorForLandmark(LINES_BODY[i][0]), -11);
                            cv::circle(frame, point2, 4, ColorForLandmark(LINES_BODY[i][1]), -11);
                        }

                        if (std::find(pushed.begin(), pushed.end(), LINES_BODY[i][0]) == pushed.end()) 
                        {
                            nlohmann::json landmarkJson = {};
                            pushed.push_back(LINES_BODY[i][0]);
                            
                            landmarkJson["index"] = LINES_BODY[i][0];
                            landmarkJson["xpos"] = landmarks_x[LINES_BODY[i][0]];
                            landmarkJson["ypos"] = landmarks_y[LINES_BODY[i][0]];

                            if (useDepth && count>0)
                            {
                                if (useSpatialLocator)
                                {
                                    landmarkJson["location.x"] = landmarks_xpos[LINES_BODY[i][0]];
                                    landmarkJson["location.y"] = landmarks_ypos[LINES_BODY[i][0]];
                                    landmarkJson["location.z"] = landmarks_zpos[LINES_BODY[i][0]];
                                }
                                else
                                {
                                    auto spatialData = computeDepth(landmarks_x[LINES_BODY[i][0]],landmarks_y[LINES_BODY[i][0]],frame.rows,depthFrameOrig);
                                    /*auto depthData = spatialData[LINES_BODY[i][0]]; 
                                    auto roi = depthData.config.roi;
                                    roi = roi.denormalize(depthFrame.cols, depthFrame.rows);*/
                                    
                                    for(auto depthData : spatialData) 
                                    {
                                        auto roi = depthData.config.roi;
                                        roi = roi.denormalize(depthFrame.cols, depthFrame.rows);

                                        landmarks_xpos[LINES_BODY[i][0]] = (int)depthData.spatialCoordinates.x;
                                        landmarks_ypos[LINES_BODY[i][0]] = (int)depthData.spatialCoordinates.y;
                                        landmarks_zpos[LINES_BODY[i][0]] = (int)depthData.spatialCoordinates.z;

                                        landmarkJson["location.x"] = landmarks_xpos[LINES_BODY[i][0]];
                                        landmarkJson["location.y"] = landmarks_ypos[LINES_BODY[i][0]];
                                        landmarkJson["location.z"] = landmarks_zpos[LINES_BODY[i][0]];
                                    }
                                }
                            }
                            bodyPose.push_back(landmarkJson);
                        }

                        if (std::find(pushed.begin(), pushed.end(), LINES_BODY[i][1]) == pushed.end()) 
                        {
                            nlohmann::json landmarkJson = {};
                            pushed.push_back(LINES_BODY[i][1]);
                            landmarkJson["index"] = LINES_BODY[i][1];
                            landmarkJson["xpos"] = landmarks_x[LINES_BODY[i][1]];
                            landmarkJson["ypos"] = landmarks_y[LINES_BODY[i][1]];

                            if (useDepth && count>0)
                            {
                                if (useSpatialLocator)
                                {
                                    landmarkJson["location.x"] = landmarks_xpos[LINES_BODY[i][1]];
                                    landmarkJson["location.y"] = landmarks_ypos[LINES_BODY[i][1]];
                                    landmarkJson["location.z"] = landmarks_zpos[LINES_BODY[i][1]];
                                }
                                else
                                {
                                    auto spatialData = computeDepth(landmarks_x[LINES_BODY[i][1]],landmarks_y[LINES_BODY[i][1]],frame.rows,depthFrameOrig); 
                                    /*auto depthData = spatialData[LINES_BODY[i][1]]; 
                                    auto roi = depthData.config.roi;
                                    roi = roi.denormalize(depthFrame.cols, depthFrame.rows);*/

                                    for(auto depthData : spatialData) 
                                    {
                                        auto roi = depthData.config.roi;
                                        roi = roi.denormalize(depthFrame.cols, depthFrame.rows);
                                        landmarks_xpos[LINES_BODY[i][1]] = (int)depthData.spatialCoordinates.x;
                                        landmarks_ypos[LINES_BODY[i][1]] = (int)depthData.spatialCoordinates.y;
                                        landmarks_zpos[LINES_BODY[i][1]] = (int)depthData.spatialCoordinates.z;

                                        landmarkJson["location.x"] = landmarks_xpos[LINES_BODY[i][1]];
                                        landmarkJson["location.y"] = landmarks_ypos[LINES_BODY[i][1]];
                                        landmarkJson["location.z"] = landmarks_zpos[LINES_BODY[i][1]];
                                    }
                                }                                
                            }
                            bodyPose.push_back(landmarkJson);
                        }
                    }
                    
                }
                bodyPoseJson["landmarks"] = bodyPose;
            }
            
            // Get Preview image
            if (getPreview && frame.cols>0 && frame.rows>0)
            {
                cv::Mat resizedMat(height, width, frame.type());
                cv::resize(frame, resizedMat, resizedMat.size(), cv::INTER_CUBIC);

                toARGB(resizedMat, frameInfo->colorPreviewData);
            }

            // SYSTEM INFORMATION
            if (retrieveInformation) bodyPoseJson["sysinfo"] = GetDeviceInfo(device);//infoJson;        
            if (useIMU) bodyPoseJson["imu"] = GetIMU(device);

            // RETURN JSON
            char* ret = (char*)::malloc(strlen(bodyPoseJson.dump().c_str())+1);
            ::memcpy(ret, bodyPoseJson.dump().c_str(),strlen(bodyPoseJson.dump().c_str()));
            ret[strlen(bodyPoseJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }
}