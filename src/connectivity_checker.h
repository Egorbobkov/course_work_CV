#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

class ConnectivityChecker {
public:
    static bool isFullyConnected(const std::vector<cv::Mat>& volume);
    bool hasInternalHoles(const std::vector<cv::Mat>& volume);
};
