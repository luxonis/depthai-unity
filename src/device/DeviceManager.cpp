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
#include "depthai/xlink/XLinkConnection.hpp"

#include "depthai-unity/device/DeviceManager.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"

// Multiple device support (up to 10)
std::shared_ptr<dai::Device> devices[10];
// Device state 
bool deviceRunning[10];

// Get device pointer
std::shared_ptr<dai::Device> GetDevice(int deviceNum)
{
    return devices[deviceNum];
}

// getter for device state
bool IsDeviceRunning(int deviceNum)
{
    return deviceRunning[deviceNum];
}

// get all connected devices and states
std::vector<dai::DeviceInfo> DAIGetAllDevices() {
    std::vector<dai::DeviceInfo> availableDevices;
    auto connectedDevices = dai::XLinkConnection::getAllConnectedDevices();
    for(const auto& d : connectedDevices) {
        availableDevices.push_back(d);
    }
    return availableDevices;
}

// Available = No booted
bool CheckForAvailableDevice(const char* deviceId)
{
    // get all connected devices and states
    std::vector<dai::DeviceInfo> allDevices = DAIGetAllDevices();

    bool deviceCheck = false;

    // check for all available devices
    // if deviceId is NULL ... pick first available device
    // if deviceId is not NULL ... pick device with deviceId (MxId) and check if it's available
    for(const auto& d : allDevices) {
        if (deviceId != NULL) 
        {
            if (strcmp(d.getMxId().c_str(),deviceId)==0 && d.state != X_LINK_BOOTED) deviceCheck = true;
        }
        else 
        {
            if (d.state != X_LINK_BOOTED) deviceCheck = true;
        }

        // in any case if device is available we don't need to check further
        if (deviceCheck) break;
    }

    return deviceCheck;
}

// start pipeline 
bool DAIStartPipeline(dai::Pipeline pipeline, int deviceNum, const char* deviceId)
{   
    bool res = false, found = false;
    std::shared_ptr<dai::Device> device;
    dai::DeviceInfo deviceInfo;
    
    // Find specific or first available device
    if (CheckForAvailableDevice(deviceId))
    {
        // Start pipeline
        if (deviceId == NULL) device = std::shared_ptr<dai::Device>(new dai::Device(pipeline));
        else 
        {
            std::tie(found, deviceInfo) = dai::Device::getDeviceByMxId(deviceId);
            if (!found) return false;
            device = std::shared_ptr<dai::Device>(new dai::Device(pipeline,deviceInfo));
        }
        
        // assign device and status
        devices[deviceNum] = device;
        deviceRunning[deviceNum] = true;
        res = true;
    }

    return res;
}

// get device system info. Needs pipeline definition. Predefined queue "sysinfo"
nlohmann::json GetDeviceInfo(std::shared_ptr<dai::Device> device)
{
    std::shared_ptr<dai::DataOutputQueue> qSysInfo;
    
    qSysInfo = device->getOutputQueue("sysinfo", 4, false);
    auto sysInfo = qSysInfo->get<dai::SystemInformation>();

    dai::SystemInformation info = *sysInfo;             

    nlohmann::json infoJson = {};

    infoJson["ddr_used"] = info.ddrMemoryUsage.used / (1024.0f * 1024.0f);
    infoJson["ddr_total"] = info.ddrMemoryUsage.total / (1024.0f * 1024.0f);
    infoJson["leoncss_heap_used"] = info.leonCssMemoryUsage.used / (1024.0f * 1024.0f);
    infoJson["leoncss_heap_total"] = info.leonCssMemoryUsage.total / (1024.0f * 1024.0f);
    infoJson["leonmss_heap_used"] = info.leonMssMemoryUsage.used / (1024.0f * 1024.0f);
    infoJson["leonmss_heap_total"] = info.leonMssMemoryUsage.total / (1024.0f * 1024.0f);
    infoJson["cmx_used"] = info.cmxMemoryUsage.used / (1024.0f * 1024.0f);
    infoJson["cmx_total"] = info.cmxMemoryUsage.total / (1024.0f * 1024.0f);
    const auto& t = info.chipTemperature;
    infoJson["chip_temp_avg"] = t.average;
    infoJson["cpu_usage"] = info.leonCssCpuUsage.average * 100;

    return infoJson;
}

// get IMU info. Needs IMU and pipeline definition. Predefined queue "imu"
nlohmann::json GetIMU(std::shared_ptr<dai::Device> device)
{
    nlohmann::json imuJson = {};

    auto imuQueue = device->getOutputQueue("imu", 50, false);
    auto imuData = imuQueue->get<dai::IMUData>();

    auto imuPackets = imuData->packets;
    for(auto& imuPacket : imuPackets) {
        auto& rVvalues = imuPacket.rotationVector;
        imuJson["I"] = rVvalues.i;
        imuJson["J"] = rVvalues.j;
        imuJson["K"] = rVvalues.k;
        imuJson["Real"] = rVvalues.real;
        imuJson["Accuracy"] = rVvalues.rotationVectorAccuracy;
    }

    return imuJson;
}


// Interface with Unity C#
extern "C"
{
    /**
    * Get list of all devices connected and status
    *
    * @returns Json with device info and status. To be shown on device manager (unity)
    */
    EXPORT_API const char* GetAllDevices()
    {
        std::vector<dai::DeviceInfo> allDevices = DAIGetAllDevices();
        nlohmann::json devicesArr = {};

        for(const auto& d : allDevices) {
            nlohmann::json deviceJson;
            deviceJson["deviceId"] = d.getMxId().c_str();
            //deviceJson["deviceName"] = d.desc.name;
            deviceJson["deviceState"] = "AVAILABLE";
            if (d.state == X_LINK_BOOTED) deviceJson["deviceState"] = "BOOTED";
            devicesArr.push_back(deviceJson);
        }
       
        // serialize json and return to Unity
        char* ret = (char*)::malloc(strlen(devicesArr.dump().c_str())+1);
        ::memcpy(ret, devicesArr.dump().c_str(),strlen(devicesArr.dump().c_str()));
        ret[strlen(devicesArr.dump().c_str())] = 0;
        return ret;
    }

    /**
    * Close device
    *
    * @param deviceNum Device selection on unity dropdown
    */

    EXPORT_API void DAICloseDevice(int deviceNum)
    {
        std::shared_ptr<dai::Device> device = GetDevice(deviceNum);
        if (device == NULL) return;

        // check device is running before closing
        if (deviceRunning[deviceNum])
        {
            deviceRunning[deviceNum] = false;
            device->close();
        }
    }
}