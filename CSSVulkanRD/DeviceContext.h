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
    CommandBuffer* cmdBuffer;

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
    void SubmitCommandBuffer(CommandBuffer* commandBuffer, bool block = false);
    void SubmitCommandBuffer(CommandBuffer* commandBuffer, VkFence* outPFence);

    void SetQueue(VkQueue queue);

    void Create(VkDevice GPU, uint32_t queueFamily, bool transientCommandPool = false);
    void Destroy();

private:
    VkCommandPool commandPool;
    std::vector<CommandBuffer*> commandBufferPool;
    CommandBuffer* createCommandBuffer(bool start);

    VkQueue  gpuQueue;
    VkDevice GPU;

    std::mutex _lock;
};