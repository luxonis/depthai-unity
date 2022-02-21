#pragma once

// std
#include <thread>

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
std::vector<dai::SpatialLocations> getSpatialInfo1(cv::Mat depthFrame, std::vector<dai::SpatialLocationCalculatorConfigData> rois, int mode, float depth_thresh_low, float depth_thresh_high);

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
std::vector<dai::SpatialLocations> computeDepth(float mx, float my, int frameRows, cv::Mat depthFrameOrig);

