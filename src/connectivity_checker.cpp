#include "connectivity_checker.h"
#include <queue>

bool ConnectivityChecker::isFullyConnected(const std::vector<cv::Mat>& volume) {
    if (volume.empty()) return false;

    int depth = volume.size();
    int height = volume[0].rows;
    int width = volume[0].cols;

    struct Voxel { int z, y, x; };

    auto inBounds = [&](int z, int y, int x) {
        return z >= 0 && z < depth && y >= 0 && y < height && x >= 0 && x < width;
    };

    std::vector<std::vector<std::vector<bool>>> visited(
            depth, std::vector<std::vector<bool>>(height, std::vector<bool>(width, false)));

    Voxel start{-1, -1, -1};
    int totalForeground = 0;
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (volume[z].at<uchar>(y, x) > 128) {
                    totalForeground++;
                    if (start.z == -1) start = {z, y, x};
                }
            }
        }
    }

    if (start.z == -1) return false; // Нет foreground

    // BFS (поиск в ширину)
    std::queue<Voxel> q;
    q.push(start);
    visited[start.z][start.y][start.x] = true;
    int connectedCount = 1;

    const Voxel dirs[6] = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}};

    while (!q.empty()) {
        Voxel v = q.front(); q.pop();

        for (auto d : dirs) {
            Voxel n = {v.z + d.z, v.y + d.y, v.x + d.x};
            if (inBounds(n.z, n.y, n.x)) {
                if (!visited[n.z][n.y][n.x] && volume[n.z].at<uchar>(n.y, n.x) > 128) {
                    visited[n.z][n.y][n.x] = true;
                    q.push(n);
                    connectedCount++;
                }
            }
        }
    }

    return connectedCount == totalForeground;
}

bool ConnectivityChecker::hasInternalHoles(const std::vector<cv::Mat>& volume) {
    if (volume.empty()) return false;

    int depth = volume.size();
    int height = volume[0].rows;
    int width = volume[0].cols;

    struct Voxel { int z, y, x; };

    auto inBounds = [&](int z, int y, int x) {
        return z >= 0 && z < depth && y >= 0 && y < height && x >= 0 && x < width;
    };

    std::vector<std::vector<std::vector<bool>>> visited(
            depth, std::vector<std::vector<bool>>(height, std::vector<bool>(width, false)));

    std::queue<Voxel> q;
    const Voxel dirs[6] = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}};

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if ((z == 0 || z == depth-1 || y == 0 || y == height-1 || x == 0 || x == width-1) &&
                    volume[z].at<uchar>(y, x) <= 128) {
                    q.push({z, y, x});
                    visited[z][y][x] = true;
                }
            }
        }
    }

    while (!q.empty()) {
        Voxel v = q.front(); q.pop();

        for (auto d : dirs) {
            Voxel n = {v.z + d.z, v.y + d.y, v.x + d.x};
            if (inBounds(n.z, n.y, n.x)) {
                if (!visited[n.z][n.y][n.x] && volume[n.z].at<uchar>(n.y, n.x) <= 128) {
                    visited[n.z][n.y][n.x] = true;
                    q.push(n);
                }
            }
        }
    }

    for (int z = 1; z < depth-1; z++) {
        for (int y = 1; y < height-1; y++) {
            for (int x = 1; x < width-1; x++) {
                if (volume[z].at<uchar>(y, x) <= 128 && !visited[z][y][x]) {
                    return true;
                }
            }
        }
    }

    return false;
}