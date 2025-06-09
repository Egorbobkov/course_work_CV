#include "connectivity_checker.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <folder_with_slices>" << std::endl;
        return 1;
    }

    std::string folder = argv[1];
    auto slices = loadSlices(folder);
    if (slices.empty()) return 1;

    uchar body_value = 255;

    std::cout << "Checking 3D connectivity..." << std::endl;
    bool connected = is3DConnected(slices, body_value);
    std::cout << "Volume is " << (connected ? "connected" : "disconnected") << std::endl;

    PorosityStats stats = computePorosityStats(slices, body_value);
    std::cout << "Porosity: " << stats.porosity * 100 << "%, "
              << "Internal pores: " << stats.pore_count << std::endl;

    std::cout << "Detecting floating islands (2D slices)..." << std::endl;
    detectFloatingIslands(slices, body_value);

    std::cout << "Detecting floating islands (3D)..." << std::endl;
    detectFloatingIslands3D(slices, body_value);
    return 0;
}
