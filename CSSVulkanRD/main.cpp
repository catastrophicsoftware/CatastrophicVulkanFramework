#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "CatastrophicVulkanFramework.h"

class app : public CatastrophicVulkanFrameworkApplication
{
public:
    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Render() {}
    virtual void DestroyResources() {}
};


int main() {
    //GraphicsDevice* pApp = new GraphicsDevice();

    app* pApp = new app();

    try {
        pApp->Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}