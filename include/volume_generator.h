#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

enum class ShapeType {
    SPHERE,
    CUBE,
    HOLLOW_SPHERE
};

class VolumeGenerator {
public:
    VolumeGenerator(int width, int height, int depth);

    std::vector<cv::Mat> generate(ShapeType type);

    void saveSlices(const std::vector<cv::Mat>& slices, const std::string& folderPath);

private:
    int width_, height_, depth_;

    std::vector<cv::Mat> generateSphere();
    std::vector<cv::Mat> generateCube();
    std::vector<cv::Mat> generateHollowSphere();
};
