#ifndef CONNECTIVITY_CHECKER_H
#define CONNECTIVITY_CHECKER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

std::vector<cv::Mat> loadSlices(const std::string& folder);
bool is3DConnected(const std::vector<cv::Mat>& volume, uchar body_value);

struct PorosityStats {
    double porosity;
    int pore_count;
};

PorosityStats computePorosityStats(const std::vector<cv::Mat>& volume, uchar body_value);
void detectFloatingIslands(const std::vector<cv::Mat>& volume, uchar body_value, int min_area = 30);
void detectFloatingIslands3D(const std::vector<cv::Mat>& volume, uchar body_value, int min_voxels = 10);

#endif
