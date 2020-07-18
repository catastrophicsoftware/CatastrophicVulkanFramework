#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>

struct CommandBuffer
{
    VkCommandBuffer handle;
    VkFence fence;
};

struct InflightFrame
{
    CommandBuffer*  cmdBuffer;

    VkSemaphore     imageAvailable;
    VkSemaphore     renderFinished;
    uint32_t        frameIndex;

    void* pPerFrameData;
};

class DeviceContext
{
public:
    DeviceContext();
    ~DeviceContext();

    CommandBuffer* GetCommandBuffer(bool begin = false);
    void Submit(CommandBuffer* commandBuffer);
    void Submit(CommandBuffer* commandBuffer, VkFence* outPFence);

    void SetQueue(VkQueue queue);

    void Create(VkDevice GPU, uint32_t queueFamily, bool transientCommandPool = false);
    void Destroy();

    void RegisterDescriptorPoolSize(VkDescriptorPoolSize poolSize);
    void CreateDescriptorPool(uint32_t maxDescriptorSets);

    VkDescriptorPool GetDescriptorPool() const;
private:
    VkCommandPool commandPool;
    std::vector<CommandBuffer*> commandBufferPool;
    CommandBuffer* createCommandBuffer(bool start);

    VkQueue  gpuQueue;
    VkDevice GPU;


    std::vector<VkDescriptorPoolSize> descriptorPoolDescriptions;
    VkDescriptorPool descriptorPool;
    uint32_t maxDescriptorSets;
    uint32_t numAllocatedDescriptorSets;

    std::mutex _lock;
};