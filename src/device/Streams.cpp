/**
* This file contains basic streams pipeline and interface for Unity scene called "Streams"
* Main goal is to show basic streams of OAK Device: color camera, mono right and left cameras, depth and disparity
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

#include "depthai-unity/device/Streams.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"

float maxDisparity;

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration 
* @returns pipeline 
*/
dai::Pipeline createStreamsPipeline(PipelineConfig *config)
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
        //stereo->setDepthAlign(dai::CameraBoardSocket::RGB);
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

        auto xoutDisp = pipeline.create<dai::node::XLinkOut>();            
        xoutDisp->setStreamName("disparity");
        stereo->disparity.link(xoutDisp->input);

        auto xoutMonoR = pipeline.create<dai::node::XLinkOut>();            
        xoutMonoR->setStreamName("monoR");
        stereo->rectifiedRight.link(xoutMonoR->input);

        auto xoutMonoL = pipeline.create<dai::node::XLinkOut>();            
        xoutMonoL->setStreamName("monoL");
        stereo->rectifiedLeft.link(xoutMonoL->input);

        maxDisparity = stereo->initialConfig.getMaxDisparity();
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
    EXPORT_API bool InitStreams(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createStreamsPipeline(config);

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
    EXPORT_API const char* StreamsResults(FrameInfo *frameInfo, bool getPreview, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum)
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
            
            // no specific information need it
            nlohmann::json streamsJson = {};

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;
            std::shared_ptr<dai::DataOutputQueue> dispQueue;
            std::shared_ptr<dai::DataOutputQueue> monoRQueue;
            std::shared_ptr<dai::DataOutputQueue> monoLQueue;

            // if preview image is requested. True in this case.
            if (getPreview) preview = device->getOutputQueue("preview",1,false);
            
            // if depth images are requested. All images.
            if (useDepth) 
            {
                depthQueue = device->getOutputQueue("depth", 1, false);
                dispQueue = device->getOutputQueue("disparity", 1, false);
                monoRQueue = device->getOutputQueue("monoR", 1, false);
                monoLQueue = device->getOutputQueue("monoL", 1, false);
            }
            
            if (getPreview)
            {
                auto imgFrames = preview->tryGetAll<dai::ImgFrame>();
                auto countd = imgFrames.size();
                if (countd > 0) {
                    auto imgFrame = imgFrames[countd-1];
                    if(imgFrame){
                        //printf("Frame - w: %d, h: %d\n", imgFrame->getWidth(), imgFrame->getHeight());
                        frame = toMat(imgFrame->getData(), imgFrame->getWidth(), imgFrame->getHeight(), 3, 1);

                        toARGB(frame,frameInfo->colorPreviewData);
                    }
                }
            }
        
            vector<std::shared_ptr<dai::ImgFrame>> imgDepthFrames,imgDispFrames,imgMonoRFrames,imgMonoLFrames;
            std::shared_ptr<dai::ImgFrame> imgDepthFrame,imgDispFrame,imgMonoRFrame,imgMonoLFrame;
            
            // In this case we allocate before Texture2D (ARGB32) and memcpy pointer data 
            if (useDepth)
            {   
                // Depth         
                imgDepthFrames = depthQueue->tryGetAll<dai::ImgFrame>();
                int count = imgDepthFrames.size();
                if (count > 0)
                {
                    imgDepthFrame = imgDepthFrames[count-1];
                    depthFrameOrig = imgDepthFrame->getFrame();
                    cv::normalize(depthFrameOrig, depthFrame, 255, 0, cv::NORM_INF, CV_8UC1);
                    cv::equalizeHist(depthFrame, depthFrame);
                    cv::cvtColor(depthFrame, depthFrame, cv::COLOR_GRAY2BGR);

                    toARGB(depthFrame,frameInfo->depthData);
                }

                // Disparity
                imgDispFrames = dispQueue->tryGetAll<dai::ImgFrame>();
                int countd = imgDispFrames.size();
                if (countd > 0)
                {
                    imgDispFrame = imgDispFrames[countd-1];
                    dispFrameOrig = imgDispFrame->getFrame();
                    dispFrameOrig.convertTo(dispFrame, CV_8UC1, 255 / maxDisparity);
                    cv::applyColorMap(dispFrame, dispFrame, cv::COLORMAP_JET);
                    
                    toARGB(dispFrame,frameInfo->disparityData);
                }

                // Mono R
                imgMonoRFrames = monoRQueue->tryGetAll<dai::ImgFrame>();
                int countr = imgMonoRFrames.size();
                if (countr > 0)
                {
                    imgMonoRFrame = imgMonoRFrames[countr-1];
                    monoRFrameOrig = imgMonoRFrame->getFrame();
                    cv::cvtColor(monoRFrameOrig, monoRFrame, cv::COLOR_GRAY2BGR);                    
                    toARGB(monoRFrame,frameInfo->rectifiedRData);
                }

                // Mono L
                imgMonoLFrames = monoLQueue->tryGetAll<dai::ImgFrame>();
                int countl = imgMonoLFrames.size();
                if (countl > 0)
                {
                    imgMonoLFrame = imgMonoLFrames[countl-1];
                    monoLFrameOrig = imgMonoLFrame->getFrame();
                    cv::cvtColor(monoLFrameOrig, monoLFrame, cv::COLOR_GRAY2BGR);                    
                    toARGB(monoLFrame,frameInfo->rectifiedLData);
                }

            }

            // SYSTEM INFORMATION
            if (retrieveInformation) streamsJson["sysinfo"] = GetDeviceInfo(device);        
            // IMU
            if (useIMU) streamsJson["imu"] = GetIMU(device);

            char* ret = (char*)::malloc(strlen(streamsJson.dump().c_str())+1);
            ::memcpy(ret, streamsJson.dump().c_str(),strlen(streamsJson.dump().c_str()));
            ret[strlen(streamsJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }


}