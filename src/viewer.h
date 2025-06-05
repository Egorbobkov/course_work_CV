#ifndef VIEWER_H
#define VIEWER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// Генерация и сохранение 3D проекций
void save3DProjections(const std::vector<cv::Mat>& volume,
                       const std::string& name,
                       bool show = true);

// Генерация и сохранение коллажа слайсов
void saveSliceCollage(const std::vector<cv::Mat>& volume,
                      const std::string& name,
                      bool show = true);

#endif