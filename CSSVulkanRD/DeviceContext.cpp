#include "DeviceContext.h"
#include "includes.h"

DeviceContext::DeviceContext()
{
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
            if (vkGetFenceStatus(GPU, buffer->fence) == VK_SUCCESS)
            {
                if (begin)
                {
                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    VULKAN_CALL_ERROR(vkBeginCommandBuffer(buffer->handle, &beginInfo), "failed to begin command buffer");
                }
                return buffer;
            }
        }

        //all command buffers are busy executing, new command buffer required
        auto cb = createCommandBuffer(begin);
        return cb;
    }
}

void DeviceContext::SubmitCommandBuffer(CommandBuffer* commandBuffer, bool block)
{
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->handle;

    vkResetFences(GPU, 1, &commandBuffer->fence);
    vkQueueSubmit(gpuQueue, 1, &submit, commandBuffer->fence);

    if (block) vkQueueWaitIdle(gpuQueue);
}

void DeviceContext::SubmitCommandBuffer(CommandBuffer* commandBuffer, VkFence* outPFence)
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
    commandBufferPool.push_back(newBuffer);

    if (begin)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VULKAN_CALL_ERROR(vkBeginCommandBuffer(newBuffer->handle, &beginInfo), "failed to begin command buffer");

        return newBuffer;
    }
    return newBuffer;
}