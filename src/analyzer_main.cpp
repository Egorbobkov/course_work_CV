#include "connectivity_checker.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Ошибка: укажите путь к папке со слайсами." << std::endl;
        std::cerr << "Пример использования: " << argv[0] << " ./slices_folder" << std::endl;
        return 1;
    }

    std::string folder = argv[1];
    auto slices = loadSlices(folder);
    if (slices.empty()) {
        std::cerr << "Не удалось загрузить слайсы из папки: " << folder << std::endl;
        return 1;
    }

    uchar body_value = 255;

    std::cout << "\nПроверка 3D-связности объекта:" << std::endl;
    bool connected = is3DConnected(slices, body_value);
    if (connected) {
        std::cout << "Объём является связным (3D)." << std::endl;
    } else {
        std::cout << "Объём НЕ является связным (3D)." << std::endl;
    }

    std::cout << "\nАнализ пористости:" << std::endl;
    PorosityStats stats = computePorosityStats(slices, body_value);
    std::cout << "Пористость: " << stats.porosity * 100 << "%\n";
    std::cout << "Количество внутренних пор: " << stats.pore_count << std::endl;

    std::cout << "\nСохранение визуализации пор..." << std::endl;

// Получаем имя папки (последний элемент пути)
    std::string folder_name = std::filesystem::path(folder).filename().string();

// Корень проекта (один уровень выше build-папки)
    std::string project_root = std::filesystem::current_path().parent_path().string();

// Сохраняем коллаж
    createBorderedCollageWithContours(slices, folder_name, project_root);


    std::cout << "\nПоиск висячих компонентов на 2D-срезах:" << std::endl;
    detectFloatingIslands(slices, body_value);

    std::cout << "\nПоиск висячих компонентов в 3D:" << std::endl;
    detectFloatingIslands3D(slices, body_value);

    std::cout << "\nАнализ завершён." << std::endl;
    return 0;
}
