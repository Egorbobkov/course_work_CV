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

# Главный исполняемый файл
add_executable(course_work_CV
        src/main.cpp
        src/volume_generator.cpp
        src/connectivity_checker.cpp
)

# Линкуем OpenCV
target_link_libraries(course_work_CV ${OpenCV_LIBS})

# Дополнительная утилита для просмотра слайсов
add_executable(slice_viewer
        src/viewer.cpp
)
target_link_libraries(slice_viewer ${OpenCV_LIBS})
