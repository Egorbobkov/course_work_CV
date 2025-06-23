#include "volume_generator.h"
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include "connectivity_checker.h"



namespace fs = std::filesystem;

std::vector<cv::Mat> VolumeGenerator::generateCube(
        CubeType type,
        int size,
        const std::vector<cv::Point3i>& holeCenters,
        int holeRadius) {

    std::vector<cv::Mat> slices;

    for (int z = 0; z < size; ++z) {
        cv::Mat slice(size, size, CV_8UC1, cv::Scalar(255));
        slices.push_back(slice);
    }

    if (type == CubeType::CubeWithCentralHole) {
        if (holeCenters.empty()) {
            std::cerr << "Error: holeCenters empty for CubeWithCentralHole" << std::endl;
            return slices;
        }
        auto c = holeCenters[0];

        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - c.x;
                    int dy = y - c.y;
                    int dz = z - c.z;
                    int dist2 = dx*dx + dy*dy + dz*dz;
                    if (dist2 <= holeRadius * holeRadius) {
                        slices[z].at<uchar>(y, x) = 0;
                    }
                }
            }
        }
    }
    else if (type == CubeType::CubeWithMultipleHoles) {
        for (const auto& c : holeCenters) {
            for (int z = 0; z < size; ++z) {
                for (int y = 0; y < size; ++y) {
                    for (int x = 0; x < size; ++x) {
                        int dx = x - c.x;
                        int dy = y - c.y;
                        int dz = z - c.z;
                        int dist2 = dx*dx + dy*dy + dz*dz;
                        if (dist2 <= holeRadius * holeRadius) {
                            slices[z].at<uchar>(y, x) = 0;
                        }
                    }
                }
            }
        }
    }

    else if (type == CubeType::CubeWithHangingStone) {
        cv::Point3i holeCenter(size / 2, size / 2, size / 2);
        int outerRadius = holeRadius;       // радиус поры (большой)
        int innerRadius = std::max(1, holeRadius / 2); // радиус "висячего белого камня"

        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - holeCenter.x;
                    int dy = y - holeCenter.y;
                    int dz = z - holeCenter.z;
                    int dist2 = dx * dx + dy * dy + dz * dz;

                    if (dist2 <= outerRadius * outerRadius) {
                        slices[z].at<uchar>(y, x) = 0;
                    }

                    if (dist2 <= innerRadius * innerRadius) {
                        slices[z].at<uchar>(y, x) = 255;
                    }
                }
            }
        }
    }

    else if (type == CubeType::CubeWithDisconnectedBodies) {
        // Весь куб изначально чёрный (0)
        for (int z = 0; z < size; ++z)
            slices[z].setTo(0);

        int cubeSize = size / 4; // Сделаем крупнее: 12 при size=50

        std::vector<cv::Point3i> origins = {
                {5, 5, 5},                   // первый куб в углу
                {size - cubeSize - 5, size - cubeSize - 5, size - cubeSize - 5} // второй в противоположном углу
        };

        for (const auto& origin : origins) {
            for (int z = 0; z < size; ++z) {
                if (z < origin.z || z >= origin.z + cubeSize) continue;

                for (int y = origin.y; y < origin.y + cubeSize && y < size; ++y) {
                    for (int x = origin.x; x < origin.x + cubeSize && x < size; ++x) {
                        slices[z].at<uchar>(y, x) = 255;
                    }
                }
            }
        }
    }

    else if (type == CubeType::CubeWithNoise) {
        for (int z = 0; z < size; ++z)
            slices[z].setTo(255);

        int noiseCount = size * size * size / 100; // плотность шума — можно регулировать

        cv::RNG rng(12345); // фиксированный seed для повторяемости

        for (int i = 0; i < noiseCount; ++i) {
            int x = rng.uniform(0, size);
            int y = rng.uniform(0, size);
            int z = rng.uniform(0, size);
            slices[z].at<uchar>(y, x) = 0;
        }
    }

    else if (type == CubeType::SolidCube) {
    }

    else if (type == CubeType::CubeWithZGap) {
        int gapStart = size / 2 - 1;  // например, 24
        int gapEnd = size / 2;       // например, 25

        for (int z = gapStart; z <= gapEnd; ++z) {
            slices[z].setTo(0);
        }
    }

    else if (type == CubeType::CubeWithThinBridge) {
        for (int z = 0; z < size; ++z) {
            slices[z].setTo(255);
        }

        cv::Point3i holeCenter(size / 2, size / 2, size / 2);
        int holeRadius = 10;

        cv::Point3i stoneCenter = holeCenter;
        int stoneRadius = 4;

        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - holeCenter.x;
                    int dy = y - holeCenter.y;
                    int dz = z - holeCenter.z;
                    int dist2 = dx * dx + dy * dy + dz * dz;
                    if (dist2 <= holeRadius * holeRadius) {
                        slices[z].at<uchar>(y, x) = 0;
                    }
                }
            }
        }

        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - stoneCenter.x;
                    int dy = y - stoneCenter.y;
                    int dz = z - stoneCenter.z;
                    int dist2 = dx * dx + dy * dy + dz * dz;
                    if (dist2 <= stoneRadius * stoneRadius) {
                        slices[z].at<uchar>(y, x) = 255;
                    }
                }
            }
        }

        for (int x = holeCenter.x - holeRadius; x <= holeCenter.x - stoneRadius; ++x) {
            int y = holeCenter.y;
            int z = holeCenter.z;
            slices[z].at<uchar>(y, x) = 255;
        }

        for (int y = holeCenter.y - holeRadius; y <= holeCenter.y - stoneRadius; ++y) {
            int x = holeCenter.x;
            int z = holeCenter.z;
            slices[z].at<uchar>(y, x) = 255;
        }

        for (int z = holeCenter.z - holeRadius; z <= holeCenter.z - stoneRadius; ++z) {
            int x = holeCenter.x;
            int y = holeCenter.y;
            slices[z].at<uchar>(y, x) = 255;
        }
    }
    // === Вычисляем пористость ===
    PorosityStats stats = computePorosityStats(slices, 255);

    // Определяем имя фигуры
    std::string cubeName;
    switch (type) {
        case CubeType::SolidCube: cubeName = "solid_cube"; break;
        case CubeType::CubeWithCentralHole: cubeName = "central_hole"; break;
        case CubeType::CubeWithMultipleHoles: cubeName = "multiple_holes"; break;
        case CubeType::CubeWithHangingStone: cubeName = "hanging_stone"; break;
        case CubeType::CubeWithDisconnectedBodies: cubeName = "disconnected_bodies"; break;
        case CubeType::CubeWithZGap: cubeName = "z_gap"; break;
        case CubeType::CubeWithThinBridge: cubeName = "thin_bridge"; break;
        case CubeType::CubeWithNoise: cubeName = "noisy"; break;
        default: cubeName = "unknown"; break;
    }

    // Путь к JSON
    std::string json_path = "../src/reference_metrics.json";

    // Загружаем текущий JSON
    nlohmann::json j;
    std::ifstream infile(json_path);
    if (infile) {
        infile >> j;
        infile.close();
    }

    // Обновляем метрику пористости
    j[cubeName]["porosity"] = stats.porosity;

    // Сохраняем обратно
    std::ofstream out(json_path);
    out << std::setw(4) << j << std::endl;

    // Финально возвращаем срезы
    return slices;
}


bool VolumeGenerator::saveSlices(const std::vector<cv::Mat>& slices, const std::string& folder) {
    fs::create_directories(folder);

    for (size_t i = 0; i < slices.size(); ++i) {
        std::string filename = folder + "/slice_" + std::to_string(i) + ".png";
        if (!cv::imwrite(filename, slices[i])) {
            std::cerr << "Failed to save " << filename << std::endl;
            return false;
        }
    }
    return true;
}
