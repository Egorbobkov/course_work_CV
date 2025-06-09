#include "volume_generator.h"
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

void createBorderedCollage(const std::vector<cv::Mat>& slices, const std::string& outputPath) {
    const int cols = 10;
    const int border_size = 1; // Толщина черной границы между слайсами
    const int slice_size = slices[0].rows;

    // Вычисляем размеры коллажа с учетом границ
    int rows = (slices.size() + cols - 1) / cols;
    int collage_width = cols * (slice_size + border_size) - border_size;
    int collage_height = rows * (slice_size + border_size) - border_size;

    // Создаем белый фон
    cv::Mat collage = cv::Mat::ones(collage_height, collage_width, CV_8UC1) * 255;

    for (size_t i = 0; i < slices.size(); ++i) {
        int row = i / cols;
        int col = i % cols;

        // Позиция для вставки с учетом границ
        int y = row * (slice_size + border_size);
        int x = col * (slice_size + border_size);

        // Вставляем слайс (белый куб с черными порами)
        cv::Rect roi(x, y, slice_size, slice_size);
        slices[i].copyTo(collage(roi));

        // Добавляем черную границу справа и снизу (кроме последних столбцов/строк)
        if (col < cols - 1) {
            cv::line(collage,
                     cv::Point(x + slice_size, y),
                     cv::Point(x + slice_size, y + slice_size - 1),
                     cv::Scalar(0), border_size);
        }
        if (row < rows - 1) {
            cv::line(collage,
                     cv::Point(x, y + slice_size),
                     cv::Point(x + slice_size - 1, y + slice_size),
                     cv::Scalar(0), border_size);
        }
    }

    cv::imwrite(outputPath, collage);
}

int main() {
    fs::path project_root = fs::current_path().parent_path();
    std::string outputDir = (project_root / "data/slices").string();
    int size = 50;
    std::cout << "Saving slices to: " << outputDir << std::endl;

    // Создаем папку для коллажей
    fs::create_directories(outputDir + "/collages");

    // Куб с одной центральной порой
    std::vector<cv::Point3i> centerHole = { {25, 25, 25} };
    auto cube1 = VolumeGenerator::generateCube(CubeType::CubeWithCentralHole, 50, centerHole, 8);
    VolumeGenerator::saveSlices(cube1, outputDir + "/single_hole");
    createBorderedCollage(cube1, outputDir + "/collages/single_hole_collage.png");

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
    createBorderedCollage(cube2, outputDir + "/collages/multiple_holes_collage.png");

    auto cubeWithStone = VolumeGenerator::generateCube(
            CubeType::CubeWithHangingStone, 50, {}, 7);
    VolumeGenerator::saveSlices(cubeWithStone, outputDir + "/hanging_stone");
    createBorderedCollage(cubeWithStone, outputDir + "/collages/hanging_stone_collage.png");

    auto disconnected = VolumeGenerator::generateCube(
            CubeType::CubeWithDisconnectedBodies, 50);
    VolumeGenerator::saveSlices(disconnected, outputDir + "/disconnected_bodies");
    createBorderedCollage(disconnected, outputDir + "/collages/disconnected_bodies_collage.png");

    auto noisy = VolumeGenerator::generateCube(CubeType::CubeWithNoise, 50);
    VolumeGenerator::saveSlices(noisy, outputDir + "/cube_noise");
    createBorderedCollage(noisy, outputDir + "/collages/cube_noise_collage.png");

    auto solidCube = VolumeGenerator::generateCube(CubeType::SolidCube, size, {}, 0);
    VolumeGenerator::saveSlices(solidCube, outputDir + "/solid_cube");
    createBorderedCollage(solidCube, outputDir + "/collages/solid_cube_collage.png");

    auto cubeWithThinBridge = VolumeGenerator::generateCube(
            CubeType::CubeWithThinBridge, 50, {}, 0);
    VolumeGenerator::saveSlices(cubeWithThinBridge, outputDir + "/thin_bridge");
    createBorderedCollage(cubeWithThinBridge, outputDir + "/collages/thin_bridge_collage.png");

    auto cubeWithZGap = VolumeGenerator::generateCube(
            CubeType::CubeWithZGap, 50, {}, 0);
    VolumeGenerator::saveSlices(cubeWithZGap, outputDir + "/z_gap");
    createBorderedCollage(cubeWithZGap, outputDir + "/collages/z_gap_collage.png");

    std::cout << "Все кубы и коллажи с границами сохранены!" << std::endl;
    return 0;
}