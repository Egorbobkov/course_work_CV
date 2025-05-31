#include "volume_generator.h"
#include <filesystem>
#include <cmath>

VolumeGenerator::VolumeGenerator(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth) {}

std::vector<cv::Mat> VolumeGenerator::generate(ShapeType type) {
    switch (type) {
        case ShapeType::SPHERE: return generateSphere();
        case ShapeType::CUBE: return generateCube();
        case ShapeType::HOLLOW_SPHERE: return generateHollowSphere();
        default: return {};
    }
}

std::vector<cv::Mat> VolumeGenerator::generateSphere() {
    std::vector<cv::Mat> slices;
    int cx = width_ / 2, cy = height_ / 2, cz = depth_ / 2;
    int r = std::min({width_, height_, depth_}) / 2 - 1;

    for (int z = 0; z < depth_; ++z) {
        cv::Mat slice(height_, width_, CV_8UC1, cv::Scalar(0));
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                int dx = x - cx, dy = y - cy, dz = z - cz;
                if (dx * dx + dy * dy + dz * dz <= r * r)
                    slice.at<uchar>(y, x) = 255;
            }
        }
        slices.push_back(slice);
    }
    return slices;
}

std::vector<cv::Mat> VolumeGenerator::generateCube() {
    std::vector<cv::Mat> slices;
    int margin = width_ / 5;

    for (int z = 0; z < depth_; ++z) {
        cv::Mat slice(height_, width_, CV_8UC1, cv::Scalar(0));
        if (z >= margin && z < depth_ - margin) {
            for (int y = margin; y < height_ - margin; ++y) {
                for (int x = margin; x < width_ - margin; ++x) {
                    slice.at<uchar>(y, x) = 255;
                }
            }
        }
        slices.push_back(slice);
    }
    return slices;
}

std::vector<cv::Mat> VolumeGenerator::generateHollowSphere() {
    std::vector<cv::Mat> slices;
    int cx = width_ / 2, cy = height_ / 2, cz = depth_ / 2;
    int rOuter = std::min({width_, height_, depth_}) / 2 - 1;
    int rInner = rOuter / 2;

    for (int z = 0; z < depth_; ++z) {
        cv::Mat slice(height_, width_, CV_8UC1, cv::Scalar(0));
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                int dx = x - cx, dy = y - cy, dz = z - cz;
                int dist2 = dx * dx + dy * dy + dz * dz;
                if (dist2 <= rOuter * rOuter && dist2 >= rInner * rInner)
                    slice.at<uchar>(y, x) = 255;
            }
        }
        slices.push_back(slice);
    }
    return slices;
}

void VolumeGenerator::saveSlices(const std::vector<cv::Mat>& slices, const std::string& folderPath) {
    std::filesystem::create_directories(folderPath);
    for (size_t i = 0; i < slices.size(); ++i) {
        std::string filename = folderPath + "/slice_" + std::to_string(i) + ".png";
        cv::imwrite(filename, slices[i]);
    }
}
