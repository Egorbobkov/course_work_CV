#include "connectivity_checker.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <queue>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>

namespace fs = std::filesystem;

std::vector<cv::Mat> loadSlices(const std::string& folder) {
    std::vector<std::pair<int, cv::Mat>> files;
    std::regex re("slice_(\\d+)\\.png");

    try {
        for (const auto& entry : fs::directory_iterator(folder)) {
            if (entry.path().extension() == ".png") {
                std::string filename = entry.path().filename().string();
                std::smatch match;
                if (std::regex_match(filename, match, re)) {
                    int index = std::stoi(match[1]);
                    cv::Mat img = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
                    if (!img.empty()) {
                        files.emplace_back(index, img);
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
        return {};
    }

    if (files.empty()) {
        std::cerr << "No valid slices found in folder: " << folder << std::endl;
        return {};
    }

    // Сортируем по индексу
    std::sort(files.begin(), files.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // Проверяем размеры всех изображений
    cv::Size first_size = files[0].second.size();
    for (const auto& [index, img] : files) {
        if (img.size() != first_size) {
            std::cerr << "Error: Slice " << index << " has different size" << std::endl;
            return {};
        }
    }

    std::vector<cv::Mat> slices;
    slices.reserve(files.size());
    for (auto& [index, img] : files) {
        slices.push_back(std::move(img));
    }

    std::cout << "Загрузка " << slices.size() << " срезов из " << folder
              << " (размер: " << first_size.height << "×" << first_size.width << ")" << std::endl;

    return slices;
}

bool is3DConnected(const std::vector<cv::Mat>& volume, uchar body_value) {
    if (volume.empty()) {
        std::cerr << "Error: Empty volume" << std::endl;
        return false;
    }

    const int Z = volume.size();
    const int Y = volume[0].rows;
    const int X = volume[0].cols;

    // Проверка размеров всех слайсов
    for (int z = 0; z < Z; ++z) {
        if (volume[z].rows != Y || volume[z].cols != X) {
            std::cerr << "Error: Slice " << z << " has inconsistent size" << std::endl;
            return false;
        }
    }

    // Используем BFS для обхода (более эффективен для больших объемов)
    std::vector<std::vector<std::vector<bool>>> visited(
            Z, std::vector<std::vector<bool>>(Y, std::vector<bool>(X, false)));

    std::queue<std::tuple<int, int, int>> q;

    // Находим первую точку тела в первом слое
    bool found = false;
    for (int y = 0; y < Y && !found; ++y) {
        for (int x = 0; x < X && !found; ++x) {
            if (volume[0].at<uchar>(y, x) == body_value) {
                q.emplace(0, y, x);
                visited[0][y][x] = true;
                found = true;
            }
        }
    }

    if (!found) return false; // Нет тела вообще

    // 6-связный обход (только соседние воксели)
    const int dz[] = {-1, 1, 0, 0, 0, 0};
    const int dy[] = {0, 0, -1, 1, 0, 0};
    const int dx[] = {0, 0, 0, 0, -1, 1};

    while (!q.empty()) {
        auto [z, y, x] = q.front();
        q.pop();

        for (int i = 0; i < 6; ++i) {
            int nz = z + dz[i];
            int ny = y + dy[i];
            int nx = x + dx[i];

            if (nz >= 0 && nz < Z && ny >= 0 && ny < Y && nx >= 0 && nx < X) {
                if (!visited[nz][ny][nx] && volume[nz].at<uchar>(ny, nx) == body_value) {
                    visited[nz][ny][nx] = true;
                    q.emplace(nz, ny, nx);
                }
            }
        }
    }

    // Проверяем есть ли непосещенные точки тела в последнем слое
    for (int y = 0; y < Y; ++y) {
        for (int x = 0; x < X; ++x) {
            if (volume[Z-1].at<uchar>(y, x) == body_value && !visited[Z-1][y][x]) {
                return false;
            }
        }
    }

    return true;
}

PorosityStats computePorosityStats(const std::vector<cv::Mat>& volume, uchar body_value) {
    int depth = volume.size();
    int height = volume[0].rows;
    int width = volume[0].cols;

    std::vector<std::vector<std::vector<int>>> visited(depth,
                                                       std::vector<std::vector<int>>(height, std::vector<int>(width, 0)));

    int total_voxels = 0;
    int empty_voxels = 0;
    int pore_count = 0;

    auto inBounds = [&](int z, int y, int x) {
        return 0 <= z && z < depth && 0 <= y && y < height && 0 <= x && x < width;
    };

    // Шаблон направлений для 26-связности
    const int dz[] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
    const int dy[] = {-1,  0,  1,-1, 0, 1,-1, 0, 1};
    const int dx[] = {-1,  0,  1,-1, 0, 1,-1, 0, 1};

    // Обход по всем вокселям
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uchar val = volume[z].at<uchar>(y, x);
                total_voxels++;
                if (val != body_value && visited[z][y][x] == 0) {
                    // Начинаем поиск пустой компоненты
                    bool touches_border = (z == 0 || z == depth - 1 ||
                                           y == 0 || y == height - 1 ||
                                           x == 0 || x == width - 1);

                    int local_empty_count = 0;
                    std::queue<std::tuple<int, int, int>> q;
                    q.push({z, y, x});
                    visited[z][y][x] = 1;

                    while (!q.empty()) {
                        auto [cz, cy, cx] = q.front(); q.pop();
                        local_empty_count++;

                        for (int i = 0; i < 9; ++i) {
                            for (int j = 0; j < 3; ++j) {
                                int nz = cz + dz[i];
                                int ny = cy + dy[i];
                                int nx = cx + dx[j];

                                if (!inBounds(nz, ny, nx)) continue;
                                if (visited[nz][ny][nx]) continue;
                                if (volume[nz].at<uchar>(ny, nx) == body_value) continue;

                                if (nz == 0 || nz == depth - 1 ||
                                    ny == 0 || ny == height - 1 ||
                                    nx == 0 || nx == width - 1)
                                    touches_border = true;

                                visited[nz][ny][nx] = 1;
                                q.push({nz, ny, nx});
                            }
                        }
                    }

                    empty_voxels += local_empty_count;
                    if (!touches_border) {
                        pore_count++;
                    }
                }
            }
        }
    }

    double porosity = (double)empty_voxels / total_voxels;
    return {porosity, pore_count};
}


void createBorderedCollageWithContours(const std::vector<cv::Mat>& slices,
                                       const std::string& folder_name,
                                       const std::string& project_root) {
    const int cols = 10;
    const int border_size = 1;
    const int slice_size = slices[0].rows;

    int rows = (slices.size() + cols - 1) / cols;
    int collage_width = cols * (slice_size + border_size) - border_size;
    int collage_height = rows * (slice_size + border_size) - border_size;

    // Цветной коллаж (BGR)
    cv::Mat collage = cv::Mat::ones(collage_height, collage_width, CV_8UC3) * 255;

    bool is_disconnected_case = folder_name.find("disconnected") != std::string::npos;

    for (size_t i = 0; i < slices.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        int y = row * (slice_size + border_size);
        int x = col * (slice_size + border_size);

        cv::Mat contours_img;
        cv::cvtColor(slices[i], contours_img, cv::COLOR_GRAY2BGR);

        // 🔴 Контуры пор (по инверсии)
        cv::Mat inv_binary;
        cv::threshold(slices[i], inv_binary, 127, 255, cv::THRESH_BINARY_INV);
        std::vector<std::vector<cv::Point>> pore_contours;
        cv::findContours(inv_binary, pore_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        cv::drawContours(contours_img, pore_contours, -1, cv::Scalar(0, 0, 255), 1); // красный

        if (is_disconnected_case) {
            // 🔵 Контуры тел (если это cubeWithDisconnectedBodies)
            cv::Mat body_binary;
            cv::threshold(slices[i], body_binary, 127, 255, cv::THRESH_BINARY);
            std::vector<std::vector<cv::Point>> body_contours;
            cv::findContours(body_binary, body_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            cv::drawContours(contours_img, body_contours, -1, cv::Scalar(255, 0, 0), 1); // синий
        }

        cv::Rect roi(x, y, slice_size, slice_size);
        contours_img.copyTo(collage(roi));

        // Границы между слайдами
        if (col < cols - 1) {
            cv::line(collage,
                     cv::Point(x + slice_size, y),
                     cv::Point(x + slice_size, y + slice_size - 1),
                     cv::Scalar(0, 0, 0), border_size);
        }
        if (row < rows - 1) {
            cv::line(collage,
                     cv::Point(x, y + slice_size),
                     cv::Point(x + slice_size - 1, y + slice_size),
                     cv::Scalar(0, 0, 0), border_size);
        }
    }

    std::string out_dir = project_root + "/data/output/collages/";
    fs::create_directories(out_dir);
    std::string output_path = out_dir + folder_name + "_collage_with_contours.png";
    cv::imwrite(output_path, collage);

    std::cout << "\nКоллаж с границами сохранён в: " << output_path << std::endl;
}



void detectFloatingIslands(const std::vector<cv::Mat>& volume, uchar body_value, int min_area) {
    if (volume.empty()) return;

    for (int z = 0; z < volume.size(); ++z) {
        cv::Mat binary;
        cv::compare(volume[z], body_value, binary, cv::CMP_EQ);

        cv::Mat labels, stats, centroids;
        int n_components = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8);

        for (int i = 1; i < n_components; ++i) {
            int area = stats.at<int>(i, cv::CC_STAT_AREA);
            if (area < min_area) {
                std::cout << "Обнаружены висячие участки на срезах: " << z
                          << ", связная область " << i
                          << ", площадь: " << area << " пикселей" << std::endl;
            }
        }
    }
}

struct Vec3 {
    int z, y, x;
    Vec3(int z_, int y_, int x_) : z(z_), y(y_), x(x_) {}
};

bool isInside(int z, int y, int x, int D, int H, int W) {
    return z >= 0 && z < D && y >= 0 && y < H && x >= 0 && x < W;
}

int detectFloatingIslands3D(const std::vector<cv::Mat>& volume, uchar body_value, int min_voxels) {
    const int D = volume.size();
    const int H = volume[0].rows;
    const int W = volume[0].cols;

    cv::Mat1i labels(D, H * W, int(0));
    int current_label = 1;

    std::map<int, std::vector<Vec3>> label_voxels;

    const std::vector<Vec3> directions = {
            {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0},
            {1, 0, 0}, {-1, 0, 0}
    };

    for (int z = 0; z < D; ++z) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = y * W + x;
                if (volume[z].at<uchar>(y, x) == body_value && labels(z, idx) == 0) {
                    std::queue<Vec3> q;
                    q.emplace(z, y, x);
                    labels(z, idx) = current_label;
                    label_voxels[current_label].emplace_back(z, y, x);

                    while (!q.empty()) {
                        Vec3 v = q.front(); q.pop();
                        for (const Vec3& d : directions) {
                            int nz = v.z + d.z;
                            int ny = v.y + d.y;
                            int nx = v.x + d.x;
                            if (isInside(nz, ny, nx, D, H, W) &&
                                volume[nz].at<uchar>(ny, nx) == body_value &&
                                labels(nz, ny * W + nx) == 0) {
                                labels(nz, ny * W + nx) = current_label;
                                q.emplace(nz, ny, nx);
                                label_voxels[current_label].emplace_back(nz, ny, nx);
                            }
                        }
                    }
                    current_label++;
                }
            }
        }
    }

    int floating_count = 0;

    // Используем обычный цикл без structured bindings для совместимости
    for (const auto& pair : label_voxels) {
        int label = pair.first;
        const auto& voxels = pair.second;

        bool touches_z0 = false;
        for (const auto& v : voxels) {
            if (v.z == 0) {
                touches_z0 = true;
                break;
            }
        }

        if (!touches_z0 && voxels.size() >= static_cast<size_t>(min_voxels)) {
            std::cout << "Обнаружены висячие участки в объёме: " << label
                      << " – Объём: " << voxels.size() << " вокселей" << std::endl;
            floating_count++;
        }
    }

    return floating_count;
}

void compareWithReferenceMetrics(const std::string& cube_name, bool is_connected, const PorosityStats& stats, int floating_3d_count) {
    std::ifstream in("../src/reference_metrics.json");
    if (!in) {
        std::cerr << "❌ Не удалось открыть reference_metrics.json" << std::endl;
        return;
    }

    nlohmann::json ref;
    in >> ref;

    if (!ref.contains(cube_name)) {
        std::cerr << "⚠️ Нет эталонных метрик для фигуры: " << cube_name << std::endl;
        return;
    }

    auto j = ref[cube_name];

    int internal_pores_ref = j.value("internal_pores", 0);
    int floating_parts_ref = j.value("floating_parts", 0);
    bool connected_ref = j.value("connected", false);
    double porosity_ref = j.value("porosity", -1.0);

    double porosity_diff = std::abs(stats.porosity - porosity_ref);
    bool porosity_match = (porosity_ref >= 0.0 && porosity_diff <= 0.001);
    bool connected_match = (is_connected == connected_ref);
    bool internal_pores_match = (stats.pore_count == internal_pores_ref);
    bool floating_parts_match = (floating_3d_count == floating_parts_ref);

    bool all_ok = porosity_match && connected_match && internal_pores_match && floating_parts_match;

    // === Печать в консоль ===
    std::cout << "\n🔎 Сравнение с эталонными метриками:\n";
    std::cout << "• Связность: " << (connected_match ? "✅" : "❌")
              << " (ожидалось: " << (connected_ref ? "да" : "нет") << ")\n";
    if (porosity_ref >= 0.0) {
        std::cout << "• Пористость: " << stats.porosity
                  << " (ожидалось: " << porosity_ref << ") "
                  << (porosity_match ? "✅" : "❌")
                  << " (Δ = " << porosity_diff << ")\n";
    } else {
        std::cout << "• Пористость: " << stats.porosity << " (эталон отсутствует) ⚠️\n";
    }
    std::cout << "• Внутренних пор: " << stats.pore_count
              << " (ожидалось: " << internal_pores_ref << ") "
              << (internal_pores_match ? "✅" : "❌") << "\n";
    std::cout << "• Висячих тел: " << floating_3d_count
              << " (ожидалось: " << floating_parts_ref << ") "
              << (floating_parts_match ? "✅" : "❌") << "\n";

    // === Сохраняем в JSON ===
    nlohmann::json result;
    std::ifstream comp_in("../data/output/results/" + cube_name + "_result.json");
    if (comp_in) {
        comp_in >> result;
        comp_in.close();
    }

    result[cube_name] = {
            {"matches", all_ok},
            {"connected_match", connected_match},
            {"porosity_match", porosity_match},
            {"porosity_diff", porosity_diff},
            {"internal_pores_match", internal_pores_match},
            {"floating_parts_match", floating_parts_match},
            {"actual", {
                                {"connected", is_connected},
                                {"porosity", stats.porosity},
                                {"internal_pores", stats.pore_count},
                                {"floating_parts", floating_3d_count}
                        }}
    };

    std::string output_path = "../data/output/results/" + cube_name + "_result.json";
    std::ofstream out(output_path);
    out << std::setw(4) << result << std::endl;
}
