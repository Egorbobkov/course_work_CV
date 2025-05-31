#include "connectivity_checker.h"
#include <queue>

bool ConnectivityChecker::isFullyConnected(const std::vector<cv::Mat>& volume) {
    int depth = volume.size();
    if (depth == 0) return false;

    int height = volume[0].rows;
    int width = volume[0].cols;

    // Воксель — структура координат
    struct Voxel { int z, y, x; };

    // Проверка границ
    auto inBounds = [&](int z, int y, int x) {
        return z >= 0 && z < depth && y >= 0 && y < height && x >= 0 && x < width;
    };

    std::vector<std::vector<std::vector<bool>>> visited(
            depth, std::vector<std::vector<bool>>(height, std::vector<bool>(width, false))
    );

    int totalForeground = 0;
    bool foundStart = false;
    Voxel startVoxel;

    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (volume[z].at<uchar>(y, x) == 255) {
                    ++totalForeground;
                    if (!foundStart) {
                        startVoxel = {z, y, x};
                        foundStart = true;
                    }
                }
            }
        }
    }

    if (!foundStart || totalForeground == 0) return false;

    std::queue<Voxel> q;
    q.push(startVoxel);
    visited[startVoxel.z][startVoxel.y][startVoxel.x] = true;


    if (!foundStart || totalForeground == 0) return false;

    int connectedCount = 0;
    std::vector<Voxel> directions = {
            {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };

    while (!q.empty()) {
        Voxel v = q.front(); q.pop();
        ++connectedCount;

        for (auto d : directions) {
            int nz = v.z + d.z, ny = v.y + d.y, nx = v.x + d.x;
            if (inBounds(nz, ny, nx) && !visited[nz][ny][nx] && volume[nz].at<uchar>(ny, nx) == 255) {
                visited[nz][ny][nx] = true;
                q.push({nz, ny, nx});
            }
        }
    }

    return connectedCount == totalForeground;
}

bool ConnectivityChecker::hasInternalHoles(const std::vector<cv::Mat>& volume) {
    int depth = volume.size();
    if (depth == 0) return false;

    int height = volume[0].rows;
    int width = volume[0].cols;

    struct Voxel { int z, y, x; };

    auto inBounds = [&](int z, int y, int x) {
        return z >= 0 && z < depth && y >= 0 && y < height && x >= 0 && x < width;
    };

    // Создаем 3D массив посещенных для фоновых вокселей
    std::vector<std::vector<std::vector<bool>>> visited(
            depth, std::vector<std::vector<bool>>(height, std::vector<bool>(width, false))
    );

    std::queue<Voxel> q;

    // Добавляем все фоновые воксели на гранях объёма
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Проверяем грани: если voxel на грани и фон
                if ((z == 0 || z == depth - 1 || y == 0 || y == height - 1 || x == 0 || x == width - 1)
                    && volume[z].at<uchar>(y, x) == 0 && !visited[z][y][x]) {
                    q.push({z, y, x});
                    visited[z][y][x] = true;
                }
            }
        }
    }

    std::vector<Voxel> directions = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    // BFS по фоновой области снаружи
    while (!q.empty()) {
        Voxel v = q.front(); q.pop();

        for (auto d : directions) {
            int nz = v.z + d.z, ny = v.y + d.y, nx = v.x + d.x;
            if (inBounds(nz, ny, nx) && !visited[nz][ny][nx] && volume[nz].at<uchar>(ny, nx) == 0) {
                visited[nz][ny][nx] = true;
                q.push({nz, ny, nx});
            }
        }
    }

    // Теперь проверяем, есть ли не посещённые фоновые воксели внутри (это внутренние пустоты)
    for (int z = 0; z < depth; ++z) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (volume[z].at<uchar>(y, x) == 0 && !visited[z][y][x]) {
                    // Внутренний пустотелый воксель найден
                    return true;
                }
            }
        }
    }

    return false;
}
