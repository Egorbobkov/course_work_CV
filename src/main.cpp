#include "volume_generator.h"
#include <iostream>
#include "connectivity_checker.h"

int main() {
    VolumeGenerator gen(100, 100, 100);

    auto sphere = gen.generate(ShapeType::SPHERE);
    gen.saveSlices(sphere, "../data/slices/sphere");

    auto cube = gen.generate(ShapeType::CUBE);
    gen.saveSlices(cube, "../data/slices/cube");

    auto hollow = gen.generate(ShapeType::HOLLOW_SPHERE);
    gen.saveSlices(hollow, "../data/slices/hollow_sphere");

    ConnectivityChecker checker;

    bool sphereConnected = checker.isFullyConnected(sphere);
    std::cout << "Сфера " << (sphereConnected ? "связна" : "НЕ связна") << std::endl;

    bool cubeConnected = checker.isFullyConnected(cube);
    std::cout << "Куб " << (cubeConnected ? "связен" : "НЕ связен") << std::endl;

    bool hollowConnected = checker.isFullyConnected(hollow);
    bool hollowHasHoles = checker.hasInternalHoles(hollow);

    if (hollowConnected) {
        if (hollowHasHoles) {
            std::cout << "Пустотелая сфера связна, но с внутренними пустотами" << std::endl;
        } else {
            std::cout << "Пустотелая сфера связна без внутренних пустот" << std::endl;
        }
    } else {
        std::cout << "Пустотелая сфера НЕ связна" << std::endl;
    }

    std::cout << "Генерация завершена." << std::endl;
    return 0;
}
