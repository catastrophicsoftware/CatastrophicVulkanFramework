#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>


struct CommandBuffer
{
    VkCommandBuffer handle;
    VkFence fence;
    bool recording;

    void Begin();
    void End();
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
    void Submit(CommandBuffer* commandBuffer, bool block=false);
    void Submit(CommandBuffer* commandBuffer, VkFence* outPFence);

    void SetQueue(VkQueue queue);

    void Create(VkDevice GPU, uint32_t queueFamily, bool transientCommandPool = false);
    void Destroy();

    void RegisterDescriptorPoolSize(VkDescriptorPoolSize poolSize);
    void CreateDescriptorPool(uint32_t maxDescriptorSets);
    void DestroyDescriptorPool();

    VkDescriptorPool GetDescriptorPool() const;

    void QueueCommandBuffer(CommandBuffer* finishedBuffer);
    void SubmitQueuedCommandBuffers();

    void PipelineExecutionBarrier(VkPipelineStageFlagBits sourceStage, VkPipelineStageFlagBits destStage);
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
    bool descriptorPoolCreated;

    std::vector<CommandBuffer*> finishedQueuedCommandBuffers;
};