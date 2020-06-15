#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "GraphicsDevice.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>


int main() {
    GraphicsDevice* pApp = new GraphicsDevice();

    try {
        pApp->Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

