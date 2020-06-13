#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator); 

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class CatastrophicVulkanApplication{
public:
    void Run();

private:
    GLFWwindow * window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalGPU = VK_NULL_HANDLE;
    VkDevice GPU;
    VkQueue GraphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;

    void initWindow();
    void initVulkan();
    void mainLoop();


    void cleanup();
    void createInstance();
    void createLogicalDevice();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    void CreateSurface();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    uint32_t WindowWidth = 1280;
    uint32_t WindowHeight = 1024;

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalGPU);

    bool IsDeviceSuitable(VkPhysicalDevice physicalGPU);
    void PickPhysicalGPU();

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};