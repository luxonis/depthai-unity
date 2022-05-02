/**
* This file is part an adapation of the work of Keijiro Takahashi (Unity Japan)
* Original repo: https://github.com/keijiro/DepthAITestbed
* Free license
*
* Basically we use depth information from OAK-D to create point cloud using Visual Effect Graph (Particle VFX)
* We return mono right image (rectified) and depth to Unity
*
* 1. Point cloud visualization
* There is Unity scene called "Poin" for standard point cloud visualization. In this case using mono right image (rectified) to "monorize" point cloud, 
* as it's directly aligned with depth information.
*
* 2. Hologram based effects: Matrix, Matrix2
* Using same approach we take advantge of Visual Effect Graph integration to explore more artistic effects over the point cloud
* 
* Check Unity scenes: "" to see point cloud in action
* 
* The same pipeline works for basic point cloud visualization and hologram effects
*
* Coming soon RGB-Depth aligned example
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

#include "depthai-unity/device/PointCloudVFX.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"

/**
* Pipeline creation based on streams template
*
* @param config pipeline configuration 
* @returns pipeline 
*/
dai::Pipeline createPointCloudVFXPipeline(PipelineConfig *config)
{
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::XLinkOut> xlinkOut;
    
    auto colorCam = pipeline.create<dai::node::ColorCamera>();

    // Color camera preview
    if (config->previewSizeWidth > 0 && config->previewSizeHeight > 0) 
    {
        xlinkOut = pipeline.create<dai::node::XLinkOut>();
        xlinkOut->setStreamName("preview");
        
        if (config->depthAlign > 0) colorCam->isp.link(xlinkOut->input);
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

        auto xoutMonoR = pipeline.create<dai::node::XLinkOut>();            
        xoutMonoR->setStreamName("monoR");
        stereo->rectifiedRight.link(xoutMonoR->input);
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

// Interface with C#
extern "C"
{
    /**
    * Pipeline creation based on streams template
    *
    * @param config pipeline configuration 
    * @returns pipeline 
    */
    EXPORT_API bool InitPointCloudVFX(PipelineConfig *config)
    {
        dai::Pipeline pipeline = createPointCloudVFXPipeline(config);

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
    EXPORT_API const char* PointCloudVFXResults(FrameInfo *frameInfo, bool getPreview, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum)
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
            nlohmann::json pointCloudVFXJson = {};

            std::shared_ptr<dai::DataOutputQueue> preview;
            std::shared_ptr<dai::DataOutputQueue> depthQueue;
            std::shared_ptr<dai::DataOutputQueue> dispQueue;
            std::shared_ptr<dai::DataOutputQueue> monoRQueue;
            std::shared_ptr<dai::DataOutputQueue> monoLQueue;

            // if preview image is requested. Optional in this case.
            if (getPreview) preview = device->getOutputQueue("preview",1,false);
            
            // if depth images are requested. Depth and Mono right rectified 
            if (useDepth) 
            {
                depthQueue = device->getOutputQueue("depth", 1, false);
                monoRQueue = device->getOutputQueue("monoR", 1, false);
            }
        
            vector<std::shared_ptr<dai::ImgFrame>> imgDepthFrames,imgDispFrames,imgMonoRFrames,imgMonoLFrames;
            std::shared_ptr<dai::ImgFrame> imgDepthFrame,imgDispFrame,imgMonoRFrame,imgMonoLFrame;

            cv::Mat monoR;
            // In this case following Keijiro approach we return directly pointers to depth (CV_16UC1 / R16) and rectifiedR (CV_8UC1 / R8)
            
            std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> t1;
            std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> t2;

            bool match = false;

            if (getPreview)
            {
                imgFrame = preview->get<dai::ImgFrame>();
                t1 = imgFrame->getTimestamp();
            }

            if (useDepth)
            {            
                imgDepthFrame = depthQueue->get<dai::ImgFrame>();
                
                t2 = imgDepthFrame->getTimestamp();

                while (imgDepthFrame)
                {
                    t2 = imgDepthFrame->getTimestamp();
                    if (t1-t2 < milliseconds(20))
                    {
                        match = true;
                        break;
                    }
                    else
                    {
                        imgDepthFrame = depthQueue->get<dai::ImgFrame>();
                    }
                }

                if (match)
                {
                    auto fp16 = (const unsigned short*)imgDepthFrame->getData().data();         
                    for (int i = 0; i < 640*360/*640*400*/; i++) {
                        ((unsigned short*)frameInfo->depthData)[i] = (unsigned short)fp16[i];
                    }
                }
            }

            if (match)
            {
                frame = imgFrame->getCvFrame();
                toARGB(frame,frameInfo->colorPreviewData);
            }

            // SYSTEM INFORMATION
            if (retrieveInformation) pointCloudVFXJson["sysinfo"] = GetDeviceInfo(device);        
            if (useIMU) pointCloudVFXJson["imu"] = GetIMU(device);

            char* ret = (char*)::malloc(strlen(pointCloudVFXJson.dump().c_str())+1);
            ::memcpy(ret, pointCloudVFXJson.dump().c_str(),strlen(pointCloudVFXJson.dump().c_str()));
            ret[strlen(pointCloudVFXJson.dump().c_str())] = 0;

            return ret;
        }

        char* ret = (char*)::malloc(strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ::memcpy(ret, "{\"error\":\"DEVICE_NOT_RUNNING\"}",strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}"));
        ret[strlen("{\"error\":\"DEVICE_NOT_RUNNING\"}")] = 0;
        return ret;
    }


}