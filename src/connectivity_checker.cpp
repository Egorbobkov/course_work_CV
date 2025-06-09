#include "connectivity_checker.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <queue>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <set>

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

    std::cout << "Loaded " << slices.size() << " slices from " << folder
              << " (size: " << first_size << ")" << std::endl;
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
                std::cout << "Floating island detected in slice " << z
                          << ", component " << i
                          << ", area: " << area << " pixels" << std::endl;
            }
        }
    }
}

#include <queue>
#include <map>
#include <set>

// Вспомогательная структура для 3D координат
struct Vec3 {
    int z, y, x;
    Vec3(int z_, int y_, int x_) : z(z_), y(y_), x(x_) {}
};

// Проверка выхода за границы
bool isInside(int z, int y, int x, int D, int H, int W) {
    return z >= 0 && z < D && y >= 0 && y < H && x >= 0 && x < W;
}

void detectFloatingIslands3D(const std::vector<cv::Mat>& volume, uchar body_value, int min_voxels) {
    const int D = volume.size();
    const int H = volume[0].rows;
    const int W = volume[0].cols;

    cv::Mat1i labels(D, H * W, int(0));  // 3D разметка: по сути — [D][H][W], уплощено
    int current_label = 1;

    std::map<int, std::vector<Vec3>> label_voxels;
    std::set<int> touches_base;

    const std::vector<Vec3> directions = {
            {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0},
            {1, 0, 0}, {-1, 0, 0}
    };

    for (int z = 0; z < D; ++z) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = y * W + x;
                if (volume[z].at<uchar>(y, x) == body_value && labels(z, idx) == 0) {
                    // Новый компонент
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

    std::cout << "=== 3D Floating Island Detection ===" << std::endl;

    for (const auto& [label, voxels] : label_voxels) {
        bool touches_z0 = false;
        for (const auto& v : voxels) {
            if (v.z == 0) {
                touches_z0 = true;
                break;
            }
        }

        if (!touches_z0 && voxels.size() >= static_cast<size_t>(min_voxels)) {
            std::cout << "Floating island (3D) detected: label " << label
                      << ", volume: " << voxels.size() << " voxels" << std::endl;
        }
    }
}
