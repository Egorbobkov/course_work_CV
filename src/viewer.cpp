#include <opencv2/opencv.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <regex>  // для удобного парсинга цифр из имени файла

namespace fs = std::filesystem;

// функция для извлечения числа из имени "slice_123.png"
int extractSliceNumber(const std::string& filename) {
    std::regex re("slice_(\\d+)\\.png");
    std::smatch match;
    if (std::regex_search(filename, match, re)) {
        return std::stoi(match[1]);
    }
    return -1; // если не найдено, можно вернуть -1 или другое число
}

std::vector<cv::Mat> loadSlices(const std::string& folderPath) {
    std::vector<std::pair<std::string, cv::Mat>> temp;

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".png") {
            std::string filename = entry.path().filename().string();
            temp.emplace_back(filename, cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE));
        }
    }

    std::sort(temp.begin(), temp.end(), [](const auto& a, const auto& b) {
        return extractSliceNumber(a.first) < extractSliceNumber(b.first);
    });

    std::vector<cv::Mat> slices;
    for (auto& p : temp) {
        slices.push_back(p.second);
    }
    return slices;
}


cv::Mat createCollage(const std::vector<cv::Mat>& images, int imagesPerRow, int thumbWidth, int thumbHeight) {
    int rows = (images.size() + imagesPerRow - 1) / imagesPerRow;

    cv::Mat collage = cv::Mat::zeros(rows * thumbHeight, imagesPerRow * thumbWidth, CV_8UC1);

    for (size_t i = 0; i < images.size(); ++i) {
        cv::Mat resized;
        cv::resize(images[i], resized, cv::Size(thumbWidth, thumbHeight));
        int row = i / imagesPerRow;
        int col = i % imagesPerRow;

        resized.copyTo(collage(cv::Rect(col * thumbWidth, row * thumbHeight, thumbWidth, thumbHeight)));
    }
    return collage;
}

int main() {
    std::vector<std::string> volumes = {
            "../data/slices/sphere",
            "../data/slices/cube",
            "../data/slices/hollow_sphere"
    };

    int currentVolume = 0;
    int imagesPerRow = 8;
    int thumbWidth = 64;
    int thumbHeight = 64;

    while (true) {
        auto slices = loadSlices(volumes[currentVolume]);

        if (slices.empty()) {
            std::cout << "Не удалось загрузить слайсы из " << volumes[currentVolume] << std::endl;
            return 1;
        }

        cv::Mat collage = createCollage(slices, imagesPerRow, thumbWidth, thumbHeight);

        cv::imshow("Slice Viewer - Volume: " + volumes[currentVolume], collage);
        int key = cv::waitKey(0);

        if (key == 27) break; // Esc — выход

        if (key == 'w') { // Следующий объём
            currentVolume = (currentVolume + 1) % volumes.size();
        } else if (key == 's') { // Предыдущий объём
            currentVolume = (currentVolume - 1 + volumes.size()) % volumes.size();
        }
    }

    return 0;
}
