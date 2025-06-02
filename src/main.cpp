#include "volume_generator.h"
#include "connectivity_checker.h"
#include "viewer.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    fs::create_directories("../output");

    VolumeGenerator gen(100, 100, 100);
    ConnectivityChecker checker;

    auto sphere = gen.generate(ShapeType::SPHERE);
    auto cube = gen.generate(ShapeType::CUBE);
    auto hollow = gen.generate(ShapeType::HOLLOW_SPHERE);

    std::cout << "Результаты:\n";
    std::cout << "Сфера: " << (checker.isFullyConnected(sphere) ? "связна" : "не связна")
              << ", " << (!checker.hasInternalHoles(sphere) ? "нет полостей" : "есть полости") << "\n";
    std::cout << "Куб: " << (checker.isFullyConnected(cube) ? "связен" : "не связен")
              << ", " << (!checker.hasInternalHoles(cube) ? "нет полостей" : "есть полости") << "\n";
    std::cout << "Пустотелая сфера: " << (checker.isFullyConnected(hollow) ? "связна" : "не связна")
              << ", " << (checker.hasInternalHoles(hollow) ? "есть полости" : "нет полостей") << "\n";


    save3DProjections(sphere, "Sphere");
    saveSliceCollage(sphere, "Sphere");


    save3DProjections(cube, "Cube");
    saveSliceCollage(cube, "Cube");


    save3DProjections(hollow, "Hollow_Sphere");
    saveSliceCollage(hollow, "Hollow_Sphere");

    return 0;
}