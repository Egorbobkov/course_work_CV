#include "viewer.h"
#include <iostream>

using namespace cv;
using namespace std;

void save3DProjections(const vector<Mat>& volume,
                       const string& name,
                       bool show)
{
    if (volume.empty()) {
        cerr << "Error: Empty volume for 3D projections!" << endl;
        return;
    }

    // Создаем 3D проекции
    Mat xy = Mat::zeros(volume[0].rows, volume[0].cols, CV_32F);
    Mat xz = Mat::zeros(volume.size(), volume[0].cols, CV_32F);
    Mat yz = Mat::zeros(volume.size(), volume[0].rows, CV_32F);

    for (size_t z = 0; z < volume.size(); z++) {
        Mat slice;
        if (volume[z].channels() == 3) {
            cvtColor(volume[z], slice, COLOR_BGR2GRAY);
        } else {
            slice = volume[z].clone();
        }
        slice.convertTo(slice, CV_32F, 1.0/255.0);

        for (int y = 0; y < slice.rows; y++) {
            for (int x = 0; x < slice.cols; x++) {
                float val = slice.at<float>(y, x);
                xy.at<float>(y, x) += val;
                xz.at<float>(z, x) += val;
                yz.at<float>(z, y) += val;
            }
        }
    }

    normalize(xy, xy, 0, 255, NORM_MINMAX);
    normalize(xz, xz, 0, 255, NORM_MINMAX);
    normalize(yz, yz, 0, 255, NORM_MINMAX);

    Mat xy8, xz8, yz8;
    xy.convertTo(xy8, CV_8U);
    xz.convertTo(xz8, CV_8U);
    yz.convertTo(yz8, CV_8U);

    resize(xy8, xy8, Size(300, 300));
    resize(xz8, xz8, Size(300, 300));
    resize(yz8, yz8, Size(300, 300));

    // Собираем проекции в одну картинку
    Mat projections;
    vector<Mat> proj_mats = {xy8, xz8, yz8};
    hconcat(proj_mats.data(), proj_mats.size(), projections);

    // Добавляем подписи
    putText(projections, "XY", Point(100, 320),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255), 2);
    putText(projections, "XZ", Point(400, 320),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255), 2);
    putText(projections, "YZ", Point(700, 320),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255), 2);

    // Сохраняем и показываем
    string filename = "../output/" + name + "_3d.png";
    imwrite(filename, projections);

    if (show) {
        imshow(name + " 3D Projections", projections);
        waitKey(0);
    }
}

void saveSliceCollage(const vector<Mat>& volume,
                      const string& name,
                      bool show)
{
    if (volume.empty()) {
        cerr << "Error: Empty volume for slice collage!" << endl;
        return;
    }

    // Подготовка слайсов
    vector<Mat> small_slices;
    const int thumb_size = 30; // Увеличим размер для лучшей видимости

    for (const auto& slice : volume) {
        Mat img;
        if (slice.channels() == 3) {
            cvtColor(slice, img, COLOR_BGR2GRAY);
        } else {
            img = slice.clone();
        }
        img.convertTo(img, CV_8U);

        Mat with_contours = img.clone();
        // with_contours = drawContoursOnSlice(img); // Раскомментировать если есть

        Mat small;
        resize(with_contours, small, Size(thumb_size, thumb_size));
        small_slices.push_back(small);
    }

    Mat collage;
    vector<Mat> rows;
    const int cols = 10;

    for (int i = 0; i < 10; ++i) {
        vector<Mat> row(small_slices.begin() + i*cols,
                        small_slices.begin() + (i+1)*cols);
        Mat row_img;
        hconcat(row.data(), row.size(), row_img);
        rows.push_back(row_img);
    }
    vconcat(rows.data(), rows.size(), collage);

    Mat result(collage.rows + 40, collage.cols, CV_8UC3, Scalar(0));
    putText(result, name + " Slices", Point(10, 30),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);

    Mat roi(result, Rect(0, 40, collage.cols, collage.rows));
    cvtColor(collage, collage, COLOR_GRAY2BGR);
    collage.copyTo(roi);

    string filename = "../output/" + name + "_slices.png";
    imwrite(filename, result);

    if (show) {
        imshow(name + " Slices", result);
        waitKey(0);
    }
}