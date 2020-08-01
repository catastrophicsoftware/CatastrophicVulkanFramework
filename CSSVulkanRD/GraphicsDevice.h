#pragma once
#include "includes.h"
#include <functional>

const int MAX_FRAMES_IN_FLIGHT = 2;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator); 


struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    uint32_t graphicsQueueCount;

    std::optional<uint32_t> presentFamily;

    std::optional<uint32_t> transferFamily;
    uint32_t transferQueueCount;

    std::optional<uint32_t> computeFamily;
    uint32_t computeQueueCount;

    bool isComplete();
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Shader;
class GPUMemoryManager;
class DeviceContext;
struct InflightFrame;
class PipelineState;
class RenderPass;

class GraphicsDevice
{
public:
    GraphicsDevice(GLFWwindow* pAppWindow);
    ~GraphicsDevice();

    void InitializeVulkan();
    void ShutdownVulkan();

    VkDevice GetGPU() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkPhysicalDeviceProperties GetDeviceProperties() const;

    void ResizeFramebuffer();

    void BeginRenderPass();
    void EndRenderPass();

    void WaitForGPUIdle();
    void DrawFrame();
    int PrepareFrame();

    InflightFrame* GetCurrentFrame(); //likely an oversimplification

    std::shared_ptr<GPUMemoryManager> GetMainGPUMemoryAllocator() const;

    void PrimaryGraphicsQueueSubmit(VkSubmitInfo submitInfo, bool block=false);
    void PrimaryTransferQueueSubmit(uint32_t transferQueueIndex, VkSubmitInfo submitInfo, bool block=false);

    VkQueue GetTransferQueue(uint32_t index);
    VkQueue GetComputeQueue(uint32_t index);

    std::shared_ptr<DeviceContext> GetTransferContext() const;

    std::shared_ptr<DeviceContext> ImmediateContext;
    std::shared_ptr<DeviceContext> TransferContext;

    std::shared_ptr<DeviceContext> CreateDeviceContext(VkQueueFlags queueType, bool transient=false);

    void SetPipelineState(PipelineState* pState);

    uint32_t       GetSwapchainFramebufferCount() const;
    VkExtent2D     GetSwapchainExtent() const;
    RenderPass*   GetRenderPass() const;
    PipelineState* GetPipelineState() const;

    void SetPipelineStateRecreateCallback(std::function<void()> callback);
private:
    GLFWwindow* pApplicationWindow;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalGPU = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;
    VkDevice GPU;
    
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;

    //VkRenderPass renderPass;
    RenderPass* primaryRenderPass;


    PipelineState* pPipelineState;

    VkQueue primaryGraphicsQueue; //this is queue index 0 of the GPUs main graphics queue family

    std::vector<VkQueue> transferQueues;
    std::vector<VkQueue> computeQueues;

    std::shared_ptr<DeviceContext> immediateContext;
    std::shared_ptr<DeviceContext> transferContext;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    void initVulkan();

    void cleanup();
    void createInstance();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPools();

    void recreateSwapChain();
    void cleanupSwapchain();
    void CreateSurface();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    bool checkValidationLayerSupport();
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    std::function<void()> recreatePipelineStateCallback;

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalGPU);
    bool IsDeviceSuitable(VkPhysicalDevice physicalGPU);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalGPU);
    void PickPhysicalGPU();

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalGPU);
    VkSurfaceFormatKHR      ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR        ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D              ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    std::vector<VkImage>         swapChainImages;
    std::vector<VkImageView>     swapChainImageViews;
    std::vector<VkFramebuffer>   swapChainFramebuffers;
    //----

    size_t currentFrame = 0;
    bool framebufferResized = false;
    uint32_t imageIndex = 0;

    std::vector<InflightFrame*> inflightFrames;
    InflightFrame* GetAvailableFrame();
    InflightFrame* CreateInflightFrame();
    InflightFrame* pActiveFrame = nullptr;
    VkFence acquireImageFence;

    std::shared_ptr<GPUMemoryManager> memoryManager;
    void initializeMainMemoryManager();

    VkPhysicalDeviceProperties gpuProperties;
    void GetGPUProperties();

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};