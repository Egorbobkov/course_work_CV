#ifndef CONNECTIVITY_CHECKER_H
#define CONNECTIVITY_CHECKER_H

#include <vector>
#include <opencv2/opencv.hpp>

class ConnectivityChecker {
public:
    bool isFullyConnected(const std::vector<cv::Mat>& volume);
    bool hasInternalHoles(const std::vector<cv::Mat>& volume);
};

#endif