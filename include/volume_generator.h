#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

enum class CubeType {
    CubeWithCentralHole,
    CubeWithMultipleHoles,
    CubeWithHangingStone,
    CubeWithDisconnectedBodies,
    CubeWithNoise,
    SolidCube,
    CubeWithThinBridge
};

class VolumeGenerator {
public:
    static std::vector<cv::Mat> generateCube(
            CubeType type,
            int size = 50,
            const std::vector<cv::Point3i>& holeCenters = {},
            int holeRadius = 5);

    // Сохранение срезов в указанную папку
    static bool saveSlices(const std::vector<cv::Mat>& slices, const std::string& folder);
};
