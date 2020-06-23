#pragma once
#include "includes.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator); 

struct VertexPositionColor
{
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
};

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


struct InflightFrame
{
    VkCommandBuffer cmdBuffer;
    VkFence         cmdFence;
    VkSemaphore     imageAvailable;
    VkSemaphore     renderFinished;
};

struct CommandBuffer
{
    VkCommandBuffer commandBuffer;
    VkFence commandBufferFence;
};

class DeviceContext
{
public:
    DeviceContext(VkDevice GPU, QueueFamilyIndices queueIndices);
    ~DeviceContext();

    CommandBuffer* GetCommandBuffer(bool begin=false);

    //todo: resource freeing
private:
    VkCommandPool commandPool;
    std::vector<CommandBuffer*> commandBufferPool;

    VkDevice GPU;
};

class Shader;
class GPUMemoryManager;

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
    void PrepareFrame();

    void SetPushConstants(VkShaderStageFlags flags, size_t size, const void* pConstantData);

    VkCommandBuffer GetActiveCommandBuffer() const; //likely an oversimplification

    std::shared_ptr<GPUMemoryManager> GetMainGPUMemoryAllocator() const;

    VkCommandPool GetPrimaryCommandPool() const;

    void PrimaryGPUQueueSubmit(VkSubmitInfo submitInfo, bool block=false);
private:
    Shader* pShader;

    GLFWwindow* pApplicationWindow;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalGPU = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;
    VkDevice GPU;
    
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout descriptorSetLayout;

    VkQueue GraphicsQueue; //todo: refactor into "primaryGraphicsQueue"
    VkCommandPool commandPool;

    VkQueue primaryTransferQueue;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    void initVulkan();

    void cleanup();
    void createInstance();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPools();
    void createDescriptorSetLayout();
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

    bool pipelineDirty;

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
    InflightFrame* GetAvailableCommandBuffer();
    InflightFrame* CreateInflightFrame();
    InflightFrame* pActiveCommandBuffer = nullptr;
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