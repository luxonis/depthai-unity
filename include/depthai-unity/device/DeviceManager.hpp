#pragma once

// std
#include <thread>

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