#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "CSSVulkanApplication.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>


int main() {
    CatastrophicVulkanApplication* pApp = new CatastrophicVulkanApplication();

    try {
        pApp->Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

