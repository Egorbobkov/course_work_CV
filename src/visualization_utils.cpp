#include "visualization_utils.h"
#include <stdexcept>

cv::Mat drawContoursOnSlice(const cv::Mat& inputSlice,
                            int threshold,
                            const cv::Scalar& contourColor,
                            int thickness) {
    // Проверка входных данных
    if (inputSlice.empty()) {
        return cv::Mat();
    }

    // Конвертация в grayscale если нужно
    cv::Mat graySlice;
    if (inputSlice.channels() == 3) {
        cv::cvtColor(inputSlice, graySlice, cv::COLOR_BGR2GRAY);
    } else {
        graySlice = inputSlice.clone();
    }

    // Бинаризация
    cv::Mat binary;
    cv::threshold(graySlice, binary, threshold, 255, cv::THRESH_BINARY);

    // Конвертация в цветное для рисования
    cv::Mat colorSlice;
    cv::cvtColor(graySlice, colorSlice, cv::COLOR_GRAY2BGR);

    // Поиск и рисование контуров
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::drawContours(colorSlice, contours, -1, contourColor, thickness);

    return colorSlice;
}

cv::Mat createCollage(const std::vector<cv::Mat>& images,
                      int gridCols,
                      int gridRows,
                      const cv::Scalar& backgroundColor) {
    // Проверка входных данных
    if (images.empty()) {
        throw std::invalid_argument("Input images vector is empty");
    }
    if (gridCols <= 0) {
        throw std::invalid_argument("gridCols must be positive");
    }

    // Автоматический расчет строк если не указан
    if (gridRows <= 0) {
        gridRows = (static_cast<int>(images.size()) + gridCols - 1) / gridCols;
    }

    const int imgWidth = images[0].cols;
    const int imgHeight = images[0].rows;
    const int type = images[0].type();

    // Создаем коллаж с указанным фоном
    cv::Mat collage(imgHeight * gridRows, imgWidth * gridCols, type, backgroundColor);

    // Заполняем коллаж изображениями
    for (size_t i = 0; i < images.size(); ++i) {
        if (i >= static_cast<size_t>(gridCols * gridRows)) {
            break;
        }

        const int row = static_cast<int>(i) / gridCols;
        const int col = static_cast<int>(i) % gridCols;

        cv::Rect roi(col * imgWidth, row * imgHeight, imgWidth, imgHeight);

        // Приводим изображение к нужному формату если необходимо
        if (images[i].size() != images[0].size() || images[i].type() != type) {
            cv::Mat temp;
            cv::resize(images[i], temp, cv::Size(imgWidth, imgHeight));
            if (images[i].type() != type) {
                temp.convertTo(temp, type);
            }
            temp.copyTo(collage(roi));
        } else {
            images[i].copyTo(collage(roi));
        }
    }

    return collage;
}