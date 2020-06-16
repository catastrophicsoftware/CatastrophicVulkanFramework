#pragma once
#include "includes.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator); 
VkShaderModule createShader(VkDevice GPU,const std::vector<char>& shaderBytecode);

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
    std::optional<uint32_t> presentFamily;

    bool isComplete();
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Shader;
class GraphicsDevice
{
public:
    GraphicsDevice(GLFWwindow* pAppWindow);
    ~GraphicsDevice();

    void InitializeVulkan();
    void ShutdownVulkan();

    uint32_t FindGPUMemory(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);
    VkDevice GetGPU() const;
    VkPhysicalDevice GetPhysicalDevice() const;

    void ResizeFramebuffer();

    void BeginRenderPass();
    void EndRenderPass();

    void WaitForGPUIdle();
    void DrawFrame();


    //
    Shader* pShader;
    //
private:
    GLFWwindow* pApplicationWindow;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalGPU = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;
    VkDevice GPU;
    VkQueue GraphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

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
    void createCommandBuffers();
    void createSemaphores();
    void createSyncObjects();
    void recreateSwapChain();
    void cleanupSwapchain();
    void getGPUMemoryProperties();
    void allocateCommandBuffers();
    
    size_t currentFrame = 0;
    bool framebufferResized = false;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();

    std::vector<const char*> getRequiredExtensions();

    void CreateSurface();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    bool checkValidationLayerSupport();
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalGPU);

    bool IsDeviceSuitable(VkPhysicalDevice physicalGPU);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalGPU);
    void PickPhysicalGPU();
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalGPU);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    std::vector<VkImage>         swapChainImages;
    std::vector<VkImageView>     swapChainImageViews;
    std::vector<VkFramebuffer>   swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};