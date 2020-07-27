#include "DeviceContext.h"
#include "includes.h"

DeviceContext::DeviceContext()
{
    descriptorPoolCreated = false;
}

DeviceContext::~DeviceContext()
{
}

CommandBuffer* DeviceContext::GetCommandBuffer(bool begin)
{
    if (commandBufferPool.size() == 0)
    {
        auto cb = createCommandBuffer(begin);
        return cb;
    }
    else
    {
        for (CommandBuffer* buffer : commandBufferPool)
        {
            if (!buffer->recording && vkGetFenceStatus(GPU, buffer->fence) == VK_SUCCESS)
            {
                if (begin)
                {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    VULKAN_CALL_ERROR(vkBeginCommandBuffer(buffer->handle, &beginInfo), "failed to begin command buffer");
                    buffer->recording = true;
                }
                return buffer;
            }
        }

        //all command buffers are busy executing, new command buffer required
        auto cb = createCommandBuffer(begin);
        return cb;
    }
}

void DeviceContext::Submit(CommandBuffer* commandBuffer, bool block)
{
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->handle;

    vkResetFences(GPU, 1, &commandBuffer->fence);
    vkQueueSubmit(gpuQueue, 1, &submit, commandBuffer->fence);

    if(block)
        vkWaitForFences(GPU, 1, &commandBuffer->fence, VK_TRUE, INFINITY);
}

void DeviceContext::Submit(CommandBuffer* commandBuffer, VkFence* outPFence)
{
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->handle;

    vkResetFences(GPU, 1, &commandBuffer->fence);
    vkQueueSubmit(gpuQueue, 1, &submit, commandBuffer->fence);
    outPFence = &commandBuffer->fence;
}

void DeviceContext::SetQueue(VkQueue queue)
{
    gpuQueue = queue;
}

void DeviceContext::Create(VkDevice GPU, uint32_t queueFamily, bool transientCommandPool)
{
    this->GPU = GPU;

    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = queueFamily;

    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (transientCommandPool)
        cpci.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VULKAN_CALL_ERROR(vkCreateCommandPool(GPU, &cpci, nullptr, &commandPool), "failed to create command pool");
}

void DeviceContext::Destroy()
{
    vkDestroyCommandPool(GPU, commandPool, nullptr);

    for (int i = 0; i < commandBufferPool.size(); ++i)
    {
        vkDestroyFence(GPU, commandBufferPool[i]->fence, nullptr);
        commandBufferPool.erase(commandBufferPool.begin() + i);
    }

    for (int i = 0; i < descriptorPoolDescriptions.size(); ++i)
    {
        descriptorPoolDescriptions.erase(descriptorPoolDescriptions.begin() + i);
    }
}

void DeviceContext::RegisterDescriptorPoolSize(VkDescriptorPoolSize poolSize)
{
    descriptorPoolDescriptions.push_back(poolSize);
}

void DeviceContext::CreateDescriptorPool(uint32_t maxDescriptorSets)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = descriptorPoolDescriptions.size();
    poolInfo.pPoolSizes = descriptorPoolDescriptions.data();
    poolInfo.maxSets = maxDescriptorSets;

    this->maxDescriptorSets = maxDescriptorSets;

    VULKAN_CALL_ERROR(vkCreateDescriptorPool(GPU, &poolInfo, nullptr, &descriptorPool), "failed to create descriptor pool");
    descriptorPoolCreated = true;
}

void DeviceContext::DestroyDescriptorPool()
{
    if (descriptorPoolCreated)
    {
        vkDestroyDescriptorPool(GPU, descriptorPool, nullptr);
        descriptorPoolCreated = false;
    }
}

VkDescriptorPool DeviceContext::GetDescriptorPool() const
{
    return descriptorPool;
}

void DeviceContext::QueueCommandBuffer(CommandBuffer* finishedBuffer)
{
    finishedQueuedCommandBuffers.push_back(finishedBuffer);
}

void DeviceContext::SubmitQueuedCommandBuffers()
{
    int numBuffers = finishedQueuedCommandBuffers.size();
    VkSubmitInfo submit{};

    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = numBuffers;  

    //TODO, implement signal semaphores, wait semaphores
}

void DeviceContext::PipelineExecutionBarrier(VkPipelineStageFlagBits sourceStage, VkPipelineStageFlagBits destStage)
{
    auto barrierBuffer = GetCommandBuffer(true);

    vkCmdPipelineBarrier(barrierBuffer->handle,
        sourceStage,
        destStage,
        0, 0, 0, 0, 0, 0, 0);

    barrierBuffer->End();

    Submit(barrierBuffer);
}

CommandBuffer* DeviceContext::createCommandBuffer(bool begin)
{
    CommandBuffer* newBuffer = new CommandBuffer();

    VkCommandBufferAllocateInfo cbai{};
    cbai.commandBufferCount = 1;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = commandPool;

    VULKAN_CALL_ERROR(vkAllocateCommandBuffers(GPU, &cbai, &newBuffer->handle), "failed to allocate command buffer");

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VULKAN_CALL_ERROR(vkCreateFence(GPU, &fci, nullptr, &newBuffer->fence), "failed to create command buffer fence");
    newBuffer->recording = false;
    commandBufferPool.push_back(newBuffer);

    if (begin)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VULKAN_CALL_ERROR(vkBeginCommandBuffer(newBuffer->handle, &beginInfo), "failed to begin command buffer");
        newBuffer->recording = true;
        return newBuffer;
    }
    return newBuffer;
}

void CommandBuffer::End()
{
    vkEndCommandBuffer(handle);
    recording = false;
}

void CommandBuffer::Begin()
{
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VULKAN_CALL_ERROR(vkBeginCommandBuffer(handle, &begin), "failed to begin command buffer recording");
    recording = true;
}