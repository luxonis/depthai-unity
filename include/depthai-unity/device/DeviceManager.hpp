#pragma once

// std
#include <thread>

/**
* FrameInfo contains pointers to all the images available on OAK devices. Mirroring FrameInfo on Unity.
*
* monochrome camera images (right and left)
* color camera image
* color preview camera image
* disparity image
* depth image
* rectified monochrome camera images (right and left)
*
* Data pointers could be used in two ways:
*
* 1. Allocate Texture2D on Unity, get the pointer and memcpy on plugin side
* 2. Return pointer and LoadRawTextureData on Texture2D
*
*/
struct FrameInfo
{
    unsigned int monoRWidth, monoRHeight;
    unsigned int monoLWidth, monoLHeight;
    unsigned int colorWidth, colorHeight;
    unsigned int colorPreviewWidth, colorPreviewHeight;
    unsigned int diparityWidth, disparityHeight;
    unsigned int depthWidth, depthHeight;
    unsigned int rectifiedRWidth, rectifiedRHeight;
    unsigned int rectifiedLWidth, rectifiedLHeight;
    void* monoRData;
    void* monoLData;
    void* colorData;
    void* colorPreviewData;
    void* disparityData;
    void* depthData;
    void* rectifiedRData;
    void* rectifiedLData;
};

/**
* PipelineConfig contains all the setup option for a pipeline. Mirroring PipelineConfig on Unity.
*/

struct PipelineConfig
{
    // General config
    int deviceNum;
    const char *deviceId;
    
    // Color Camera
    float colorCameraFPS;
    // 0: THE_1080_P, 1: THE_4_K, 2: THE_12_MP, 3: THE_13_MP
    int colorCameraResolution;
    bool colorCameraInterleaved;
    // 0: BGR, 1:RGB
    int colorCameraColorOrder;
    int previewSizeWidth, previewSizeHeight;
    
    int ispScaleF1, ispScaleF2;
    int manualFocus;

    // MonoR Camera
    float monoRCameraFPS; 
    int monoRCameraResolution;

    // MonoL camera
    float monoLCameraFPS; 
    int monoLCameraResolution;

    // Stereo
    int confidenceThreshold;
    bool leftRightCheck;
    bool subpixel;
    bool extendedDisparity;
    int depthAlign;
    int medianFilter;

    // NN models
    const char *nnPath1;
    const char *nnPath2;
    const char *nnPath3;

    // System information
    float rate;

    // IMU
    int freq;
    int batchReportThreshold;
    int maxBatchReports;
};

/**
* Get device smart pointer
* 
* @param deviceNum Device selection on unity dropdown
* @returns Smart pointer to dai::Device
*/
std::shared_ptr<dai::Device> GetDevice(int deviceNum);

/**
* Check if device is currently running
*
* @param deviceNum Device selection on unity dropdown
* @returns True if device is running, false otherwise
*/
bool IsDeviceRunning(int deviceNum);

/**
* Start DepthAI pipeline with specific device mxid or first available device
*
* @param pipeline DepthAI pipeline
* @param deviceNum Device selection on unity dropdown
* @param deviceId Device MxId
* @returns True if device available and start pipeline, false otherwise 
*/
bool DAIStartPipeline(dai::Pipeline pipeline, int deviceNum, const char* deviceId);

/**
* Check for available specific device or first available device
*
* @param deviceId Device MxId
* @returns True if specific device or there is any device with state different than X_LINK_BOOTED
*/
bool CheckForAvailableDevice(const char* deviceId);

// common methods to retrieve device stats and IMU
/**
* Get device system info. Needs pipeline definition
*
* @param device Smart pointer to device
* @returns Json with system info: ddr_used, ddr_total, leoncss_heap_used, leoncss_heap_total, leonmss_heap_used, leonmss_heap_total, cmx_used,cmx_total, chip_temp_avg and cpu_usage
*/
nlohmann::json GetDeviceInfo(std::shared_ptr<dai::Device> device);

/**
* Get IMU information from device. Needs device with IMU and pipeline definition
*
* @param device Smart pointer to device
* @returns Json with IMU info: I,J,K, Real and accuracy 
*/
nlohmann::json GetIMU(std::shared_ptr<dai::Device> device);