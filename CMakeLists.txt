cmake_minimum_required(VERSION 3.10)
project(course_work_CV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV
find_package(OpenCV REQUIRED)

# Include
include_directories(
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/include
)

# Главный исполняемый файл для генерации
add_executable(course_work_CV
        src/main.cpp
        src/volume_generator.cpp
        src/visualization_utils.cpp
        src/viewer.cpp
)

# Отдельный исполняемый файл для анализа
add_executable(volume_analyzer
        src/connectivity_checker.cpp
        src/visualization_utils.cpp
        src/analyzer_main.cpp
)

# Линковка для обоих исполняемых файлов
target_link_libraries(course_work_CV ${OpenCV_LIBS})
target_link_libraries(volume_analyzer ${OpenCV_LIBS})