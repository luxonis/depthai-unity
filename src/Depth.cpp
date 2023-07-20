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

#include "utility.hpp"

// Inludes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"
#include "depthai/device/Device.hpp"

#include "depthai-unity/Depth.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "nlohmann/json.hpp"


float calc_angle(float offset)
{
    float ret;
    float hfov = 1.282817f; // 73.5 Mono HFOV
    float depthWidth = 1280.0f; //640.0f //1280.0f;   // 1,3  2,3

    ret = atan(tan(hfov / 2.0) * offset / (depthWidth / 2.0));
    return ret;
}

float calc_angle2(float offset,float width)
{
    float ret;
    float hfov = 1.282817f; // 73.5 Mono HFOV
    float depthWidth = width; //1280.0f;   // 1,3  2,3

    ret = atan(tan(hfov / 2.0) * offset / (depthWidth / 2.0));
    return ret;
}

/**
* Compute spatial info
*
* @param depthFrame depth frame
* @param rois vector of regions of interest to compute depth
* @param mode compute average or min depth of roi. 0: average, 1: min
* @param depth_thresh_low depth minimum threshold
* @param depth_thresh_high depth maximum threshold
* @return vector of spatial locations
*/

std::vector<dai::SpatialLocations> getSpatialInfo1(cv::Mat depthFrame, std::vector<dai::SpatialLocationCalculatorConfigData> rois, int mode, float depth_thresh_low, float depth_thresh_high)
{
    std::vector<dai::SpatialLocations> spatialData;
    for (int i=0; i<(int)rois.size(); i++)
    {
        dai::SpatialLocations loc;
        loc.config.roi = rois[i].roi;

        auto myroi = rois[i].roi;
        myroi = myroi.denormalize(depthFrame.cols, depthFrame.rows);

        auto xmin = (int)myroi.topLeft().x;
        auto ymin = (int)myroi.topLeft().y;
        auto xmax = (int)myroi.bottomRight().x;
        auto ymax = (int)myroi.bottomRight().y;

        float cnt = 0.0;
        float sum = 0.0;
        int xMinPos = -1, yMinPos = -1;
        unsigned short minDepth = 50000;
        unsigned short finalDepth;

        if (xmin >= 1280) xmin = 1279;
        if (xmax >= 1280) xmax = 1279;
        if (ymin >= 720) ymin = 719;
        if (ymax >= 720) ymax = 719;
        
        if (ymin < 0) ymin = 0;
        if (xmin < 0) xmin = 0;
        if (xmax < 0) xmax = 0;
        if (ymax < 0) ymax = 0;
        
        for (int x = xmin; x<xmax; x++)
        {
            for (int y=ymin; y<ymax; y++)
            {
                unsigned short depthPixel;
                
                depthPixel = depthFrame.at<unsigned short>(cv::Point(x,y));
                if (depth_thresh_low < depthPixel && depthPixel < depth_thresh_high)
                {
                    cnt++;
                    sum += depthPixel;
                    if (depthPixel < minDepth)
                    {
                        xMinPos = x;
                        yMinPos = y;
                        minDepth = depthPixel;
                    }
                }
            }
        }

        if (mode == 0)
        {
            if (cnt > 0) finalDepth = sum / cnt;
            else 
            {
                finalDepth = 0;
            }
        }

        if (mode == 1) finalDepth = minDepth;

        auto xmid = (xmax - xmin) / 2 + xmin;
        auto ymid = (ymax - ymin) / 2 + ymin;
        auto dmidx = 1280/2;
        auto dmidy = 720/2;
        auto bb_x_pos = xmid - dmidx;
        auto bb_y_pos = ymid - dmidy;
        auto angle_x = calc_angle(bb_x_pos);
        auto angle_y = calc_angle(bb_y_pos);

        loc.spatialCoordinates.z = finalDepth;
        loc.spatialCoordinates.x = finalDepth * tan(angle_x);
        loc.spatialCoordinates.y = -finalDepth * tan(angle_y);

         spatialData.push_back(loc);
    }
    return spatialData;
}

/**
* Compute 3D position of ROI around image point (mx,my) using depth image
* Similar to spatialLocation node
*
* @param mx x-axis position
* @param my y-axis position
* @param frameRows normalization between rgb and depth frames
* @param depthFrameOrig depth frame
* @return vector of spatial locations
*
* @todo Replace and use proper spatial location node. Remove hardcoded values. 
*/
std::vector<dai::SpatialLocations> computeDepth(float mx, float my, int frameRows, cv::Mat depthFrameOrig)
{
    std::vector<dai::SpatialLocationCalculatorConfigData> rois;

    float ratio = 720.0f / frameRows;
    cv::Point point3 = cv::Point(mx * ratio + (1280/2 - 720/2), my * ratio);

    float roi_size = 0.02;
    float tlx = (point3.x/1280.0f)-roi_size;
    float tly = (point3.y/720.0f)-roi_size;
    if (tlx <= 0.0f) tlx = 0.01f;
    if (tly <= 0.0f) tly = 0.01f;

    float brx = (point3.x/1280.0f)+roi_size;
    float bry = (point3.y/720.0f)+roi_size;

    if (brx >= 1.0f) brx = 0.99f;
    if (bry >= 1.0f) bry = 0.99f;

    dai::Point2f topLeft(tlx, tly);
    dai::Point2f bottomRight(brx, bry);

    dai::SpatialLocationCalculatorConfigData config;
    config.roi = dai::Rect(topLeft, bottomRight);

    rois.push_back(config);

    return getSpatialInfo1(depthFrameOrig,rois,0,100,50000);
}

/**
* Compute 3D position of ROI around image point (mx,my) using depth image
* Using spatialLocation node
*
* @param depthFrame depth frame
* @param frame rgb frame
* @param mx x-axis position
* @param my y-axis position
* @return mapped rect from rgb to depth
*
*/
dai::Rect prepareComputeDepth(cv::Mat depthFrame, cv::Mat frame, float mx, float my)
{
    float ratio = (float)depthFrame.rows / frame.rows;
    cv::Point point3 = cv::Point(mx * ratio + ((float)depthFrame.cols/2 - (float)depthFrame.rows/2), my * ratio);

    float roi_size = 0.02;
    float tlx = (point3.x/(float)depthFrame.cols)-roi_size;
    float tly = (point3.y/(float)depthFrame.rows)-roi_size;
    if (tlx <= 0.0f) tlx = 0.01f;
    if (tly <= 0.0f) tly = 0.01f;

    float brx = (point3.x/(float)depthFrame.cols)+roi_size;
    float bry = (point3.y/(float)depthFrame.rows)+roi_size;

    if (brx >= 1.0f) brx = 0.99f;
    if (bry >= 1.0f) bry = 0.99f;

    dai::Point2f topLeft(tlx, tly);
    dai::Point2f bottomRight(brx, bry);

    return dai::Rect(topLeft, bottomRight);
}