#include "volume_generator.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    fs::path project_root = fs::current_path().parent_path();
    std::string outputDir = (project_root / "data/slices").string();
    int size = 50;
    std::cout << "Saving slices to: " << outputDir << std::endl;

    // Куб с одной центральной порой
    std::vector<cv::Point3i> centerHole = { {25, 25, 25} };
    auto cube1 = VolumeGenerator::generateCube(CubeType::CubeWithCentralHole, 50, centerHole, 8);
    VolumeGenerator::saveSlices(cube1, outputDir + "/single_hole");

    // Куб с несколькими порами в разных местах
    std::vector<cv::Point3i> multipleHoles = {
            {15, 15, 15},
            {35, 15, 15},
            {15, 35, 15},
            {35, 35, 15},
            {25, 25, 35}
    };
    auto cube2 = VolumeGenerator::generateCube(CubeType::CubeWithMultipleHoles, 50, multipleHoles, 6);
    VolumeGenerator::saveSlices(cube2, outputDir + "/multiple_holes");

    auto cubeWithStone = VolumeGenerator::generateCube(
            CubeType::CubeWithHangingStone, 50, {}, 7); // 7 — радиус внешней поры

    VolumeGenerator::saveSlices(cubeWithStone, outputDir + "/hanging_stone");

    auto disconnected = VolumeGenerator::generateCube(
            CubeType::CubeWithDisconnectedBodies, 50);

    VolumeGenerator::saveSlices(disconnected, outputDir + "/disconnected_bodies");

    auto noisy = VolumeGenerator::generateCube(CubeType::CubeWithNoise, 50);


    VolumeGenerator::saveSlices(noisy, outputDir + "/cube_noise");

    auto solidCube = VolumeGenerator::generateCube(CubeType::SolidCube, size, {}, 0);
    VolumeGenerator::saveSlices(solidCube, outputDir + "/solid_cube");

    auto cubeWithThinBridge = VolumeGenerator::generateCube(
            CubeType::CubeWithThinBridge, 50, {}, 0);
    VolumeGenerator::saveSlices(cubeWithThinBridge, outputDir + "/thin_bridge");



    std::cout << "Два куба с порами сохранены!" << std::endl;
    return 0;
}
