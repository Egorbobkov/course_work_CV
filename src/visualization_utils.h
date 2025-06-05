#ifndef VISUALIZATION_UTILS_H
#define VISUALIZATION_UTILS_H

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief Рисует контуры на бинарном или grayscale изображении
 * @param inputSlice Входное изображение (CV_8UC1 или CV_8UC3)
 * @param threshold Порог для бинаризации (по умолчанию 127)
 * @param contourColor Цвет контуров (по умолчанию красный)
 * @param thickness Толщина линий (по умолчанию 1)
 * @return Изображение с контурами (CV_8UC3)
 */
cv::Mat drawContoursOnSlice(const cv::Mat& inputSlice,
                            int threshold = 127,
                            const cv::Scalar& contourColor = cv::Scalar(0, 0, 255),
                            int thickness = 1);

/**
 * @brief Создает коллаж из изображений
 * @param images Вектор изображений (должны быть одного типа и размера)
 * @param gridCols Количество столбцов
 * @param gridRows Количество строк (0 для автоматического расчета)
 * @param backgroundColor Цвет фона (по умолчанию черный)
 * @return Коллаж изображений
 * @throw std::invalid_argument если images пуст или gridCols <= 0
 */
cv::Mat createCollage(const std::vector<cv::Mat>& images,
                      int gridCols,
                      int gridRows = 0,
                      const cv::Scalar& backgroundColor = cv::Scalar(0, 0, 0));

#endif // VISUALIZATION_UTILS_H