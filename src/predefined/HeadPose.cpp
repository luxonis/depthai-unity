/**
* This file contains face emotion pipeline and interface for Unity scene called "Face Emotion"
* Main goal is to perform face emotion + depth
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

// Inludes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"
#include "depthai/device/Device.hpp"

#include "depthai-unity/predefined/HeadPose.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration 
* @returns pipeline 
*/
dai::Pipeline createHeadPosePipeline(PipelineConfig *config)
{
    
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::XLinkOut> xlinkOut;
    
    auto colorCam = pipeline.create<dai::node::ColorCamera>();

    // Color camera preview
    if (config->previewSizeWidth > 0 && config->previewSizeHeight > 0) 
    {
        xlinkOut = pipeline.create<dai::node::XLinkOut>();
        xlinkOut->setStreamName("preview");
        colorCam->setPreviewSize(config->previewSizeWidth, config->previewSizeHeight);
        colorCam->preview.link(xlinkOut->input);
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
    colorCam->preview.link(nn1->input);

    // output of neural network
    auto nnOut = pipeline.create<dai::node::XLinkOut>();
    nnOut->setStreamName("detections");    
    nn1->out.link(nnOut->input);
   

    auto xlinkIn = pipeline.create<dai::node::XLinkIn>();
    xlinkIn->setStreamName("landm_in");    

    auto nn2 = pipeline.create<dai::node::NeuralNetwork>();
    nn2->setBlobPath(config->nnPath2);
    xlinkIn->out.link(nn2->input);

    auto nnOut2 = pipeline.create<dai::node::XLinkOut>();
    nnOut2->setStreamName("landm_out");    
    nn2->out.link(nnOut2->input);

    // Depth
    /*if (config->confidenceThreshold > 0)
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
    }*/

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
    EXPORT_API bool InitHeadPose(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createHeadPosePipeline(config);

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
    */

    EXPORT_API const char* HeadPoseResults(FrameInfo *frameInfo, bool getPreview, int width, int height, bool drawBestFaceInPreview, bool drawAllFacesInPreview, float faceScoreThreshold, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum)
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

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;

            // if preview image is requested. True in this case.
            if (getPreview) preview = device->getOutputQueue("preview",1,false);
            
            // face emotion results
            auto detections = device->getOutputQueue("detections",1,false);
            
            auto landm_in = device->getInputQueue("landm_in");
            auto landm_out = device->getOutputQueue("landm_out");

            // if depth images are requested. All images.
            //if (useDepth) depthQueue = device->getOutputQueue("depth", 1, false);
            
            if (getPreview)
            {
                auto imgFrames = preview->tryGetAll<dai::ImgFrame>();
                auto countd = imgFrames.size();
                if (countd > 0) {
                    auto imgFrame = imgFrames[countd-1];
                    if(imgFrame){
                        frame = toMat(imgFrame->getData(), imgFrame->getWidth(), imgFrame->getHeight(), 3, 1);
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
            std::vector<float> detData = det->getFirstLayerFp16();
            float maxScore = 0.0;
            int maxPos = 0;

            nlohmann::json facesArr = {};
            nlohmann::json bestFace = {};
            nlohmann::json headPoseArr = {};
            nlohmann::json headPoseJson = {};

            vector<std::shared_ptr<dai::ImgFrame>> imgDepthFrames;
            std::shared_ptr<dai::ImgFrame> imgDepthFrame;
            
            if(detData.size() > 0){
                int i = 0;
                while (detData[i*7] != -1.0f && i*7 < (int)detData.size()) {
                    
                    Detection d;
                    d.label = detData[i*7 + 1];
                    d.score = detData[i*7 + 2];
                    if (d.score >= faceScoreThreshold)
                    {
                        if (d.score > maxScore) 
                        {
                            maxScore = d.score;
                            maxPos = i;
                        }
                        d.x_min = detData[i*7 + 3];
                        d.y_min = detData[i*7 + 4];
                        d.x_max = detData[i*7 + 5];
                        d.y_max = detData[i*7 + 6];
                        dets.push_back(d);
                    }
                    i++;
                }
            }
            int i = 0;
            cv::Mat faceFrame;
            for(const auto& d : dets){
                int x1 = d.x_min * frame.cols;
                int y1 = d.y_min * frame.rows;
                int x2 = d.x_max * frame.cols;
                int y2 = d.y_max * frame.rows;
                int mx = x1 + ((x2 - x1) / 2);
                int my = y1 + ((y2 - y1) / 2);

                // m_mx = mx;
                // m_my = my;

                if (faceScoreThreshold <= d.score)
                {
                    if (i==maxPos)
                    {
                        bestFace["label"] = d.label;
                        bestFace["score"] = d.score;
                        bestFace["xmin"] = d.x_min;
                        bestFace["ymin"] = d.y_min;
                        bestFace["xmax"] = d.x_max;
                        bestFace["ymax"] = d.y_max;
                        bestFace["xcenter"] = mx;
                        bestFace["ycenter"] = my;
                    }

                    nlohmann::json face;

                    face["label"] = d.label;
                    face["score"] = d.score;
                    face["xmin"] = d.x_min;
                    face["ymin"] = d.y_min;
                    face["xmax"] = d.x_max;
                    face["ymax"] = d.y_max;
                    face["xcenter"] = mx;
                    face["ycenter"] = my;

                    if (x1 > 0 && y1 > 0 && x2 < 300 && y2 < 300)
                        faceFrame = frame(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
                    else  
                    {
                        if (x1 <= 0) x1 = 0;
                        if (y1 <= 0) y1 = 0;
                        if (x2 >= 300) x2 = 300;
                        if (y2 >= 300) y2 = 300;
                        faceFrame = frame(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
                    }

                    // ------------------------- SECOND STAGE - HEAD POSE

                    dai::NNData data_in;

                    auto tensor = std::make_shared<dai::RawBuffer>();
                    if (faceFrame.cols > 0 && faceFrame.rows > 0)
                    {
                        float yaw, pitch, roll;
                        cv::Mat frame2 = resizeKeepAspectRatio(faceFrame, cv::Size(60,60), cv::Scalar(0));

                        toPlanar(frame2, tensor->data);

                        landm_in->send(tensor);

                        auto detface = landm_out->get<dai::NNData>();
                        
                        nlohmann::json headPose;

                        std::vector<float> detfaceYData = detface->getLayerFp16("angle_y_fc");
                        if (detfaceYData.size() > 0)
                        {
                            if(detfaceYData.size() > 0){
                                yaw = detfaceYData[0];
                            }
                            std::vector<float> detfacePData = detface->getLayerFp16("angle_p_fc");
                            if(detfacePData.size() > 0){
                                pitch = detfacePData[0];
                            }
                            std::vector<float> detfaceRData = detface->getLayerFp16("angle_r_fc");
                            if(detfaceRData.size() > 0){
                                roll = detfaceRData[0];
                            }

                            headPose["yaw"] = yaw;
                            headPose["roll"] = roll;
                            headPose["pitch"] = pitch;
                            
                            roll = roll * 3.141596 / 180;
                            pitch = pitch * 3.141596 / 180;
                            yaw = -(yaw * 3.141596 / 180); 

                            if (drawBestFaceInPreview)
                            {
                                int size = 50;
                                int origin0 = (x2 - x1)/2;
                                int origin1 = (y2 - y1)/2;
                                // X axis (red)
                                int rx1 = size * (cos(yaw) * cos(roll)) + origin0;
                                int ry1 = size * (cos(pitch) * sin(roll) + cos(roll) * sin(pitch) * sin(yaw)) + origin1;
                                cv::line(frame, cv::Point(origin0, origin1), cv::Point(rx1, ry1), cv::Scalar(0, 0, 255), 3);

                                // Y axis (green)
                                int rx2 = size * (-cos(yaw) * sin(roll)) + origin0;
                                int ry2 = size * (-cos(pitch) * cos(roll) - sin(pitch) * sin(yaw) * sin(roll)) + origin1;
                                cv::line(frame, cv::Point(origin0, origin1), cv::Point(rx2, ry2), cv::Scalar(0, 255, 0), 3);

                                // Z axis (blue)
                                int rx3 = size * (-sin(yaw)) + origin0;
                                int ry3 = size * (cos(yaw) * sin(pitch)) + origin1;
                                cv::line(frame, cv::Point(origin0, origin1), cv::Point(rx3,ry3), cv::Scalar(255, 0, 0), 2);
                            }
                        }

                        facesArr.push_back(face);
                        headPoseArr.push_back(headPose);

                        if (drawBestFaceInPreview) cv::rectangle(frame, cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)), cv::Scalar(255,255,255));
                    }
                }
                i++;
            }

            if (getPreview && frame.cols>0 && frame.rows>0) 
            {
                cv::Mat resizedMat(height, width, frame.type());
                cv::resize(frame, resizedMat, resizedMat.size(), cv::INTER_CUBIC);

                toARGB(frame,frameInfo->colorPreviewData);
            }

            // SYSTEM INFORMATION
            if (retrieveInformation) headPoseJson["sysinfo"] = GetDeviceInfo(device);        
            // IMU
            if (useIMU) headPoseJson["imu"] = GetIMU(device);

            // RETURN JSON
            headPoseJson["best"] = bestFace;
            headPoseJson["faces"] = facesArr;
            headPoseJson["headPoses"] = headPoseArr;

            char* ret = (char*)::malloc(strlen(headPoseJson.dump().c_str())+1);
            ::memcpy(ret, headPoseJson.dump().c_str(),strlen(headPoseJson.dump().c_str()));
            ret[strlen(headPoseJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }


}