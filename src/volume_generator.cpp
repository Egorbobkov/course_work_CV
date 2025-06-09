#include "volume_generator.h"
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::vector<cv::Mat> VolumeGenerator::generateCube(
        CubeType type,
        int size,
        const std::vector<cv::Point3i>& holeCenters,
        int holeRadius) {

    std::vector<cv::Mat> slices;

    // Инициализация куба (все белые)
    for (int z = 0; z < size; ++z) {
        cv::Mat slice(size, size, CV_8UC1, cv::Scalar(255));
        slices.push_back(slice);
    }

    if (type == CubeType::CubeWithCentralHole) {
        // Берём единственный центр
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
        // Для каждого центра поры "вырубаем" черные сферы
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
        // Центральная черная пора (сфера)
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

                    // Внутри внешней черной сферы — пора
                    if (dist2 <= outerRadius * outerRadius) {
                        slices[z].at<uchar>(y, x) = 0;
                    }

                    // Внутри центральной маленькой — возвращаем белый "камень"
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

        // Размеры и позиции двух белых кубов
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
        // Весь куб изначально белый (тело)
        for (int z = 0; z < size; ++z)
            slices[z].setTo(255);

        // Добавим случайные черные пиксели (шум)
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
        // Весь куб изначально белый (уже задано при инициализации)
        // Ничего дополнительно делать не нужно
    }

    else if (type == CubeType::CubeWithZGap) {
        // Определим, какие Z-слайсы будут пустыми (например, 24 и 25 из 50)
        int gapStart = size / 2 - 1;  // например, 24
        int gapEnd = size / 2;       // например, 25

        for (int z = gapStart; z <= gapEnd; ++z) {
            slices[z].setTo(0);  // делаем полностью чёрными
        }
    }

    else if (type == CubeType::CubeWithThinBridge) {
        // Заполняем весь куб белым (тело)
        for (int z = 0; z < size; ++z) {
            slices[z].setTo(255);
        }

        cv::Point3i holeCenter(size / 2, size / 2, size / 2);
        int holeRadius = 10;

        cv::Point3i stoneCenter = holeCenter; // в центре поры
        int stoneRadius = 4;

        // Вырезаем большую черную пору (дырку)
        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - holeCenter.x;
                    int dy = y - holeCenter.y;
                    int dz = z - holeCenter.z;
                    int dist2 = dx * dx + dy * dy + dz * dz;
                    if (dist2 <= holeRadius * holeRadius) {
                        slices[z].at<uchar>(y, x) = 0; // черная пора
                    }
                }
            }
        }

        // Нарисовать висячий камень внутри поры (маленький белый шар)
        for (int z = 0; z < size; ++z) {
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int dx = x - stoneCenter.x;
                    int dy = y - stoneCenter.y;
                    int dz = z - stoneCenter.z;
                    int dist2 = dx * dx + dy * dy + dz * dz;
                    if (dist2 <= stoneRadius * stoneRadius) {
                        slices[z].at<uchar>(y, x) = 255; // белый камень
                    }
                }
            }
        }

        // Добавляем тонкие нити — белые линии от камня к телу по трём осям
        // Горизонтальная нить (X)
        for (int x = holeCenter.x - holeRadius; x <= holeCenter.x - stoneRadius; ++x) {
            int y = holeCenter.y;
            int z = holeCenter.z;
            slices[z].at<uchar>(y, x) = 255;
        }

        // Вертикальная нить (Y)
        for (int y = holeCenter.y - holeRadius; y <= holeCenter.y - stoneRadius; ++y) {
            int x = holeCenter.x;
            int z = holeCenter.z;
            slices[z].at<uchar>(y, x) = 255;
        }

        // Глубина (Z)
        for (int z = holeCenter.z - holeRadius; z <= holeCenter.z - stoneRadius; ++z) {
            int x = holeCenter.x;
            int y = holeCenter.y;
            slices[z].at<uchar>(y, x) = 255;
        }
    }

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
