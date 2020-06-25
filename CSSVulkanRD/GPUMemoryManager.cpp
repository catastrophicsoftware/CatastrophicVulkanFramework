#include "GPUMemoryManager.h"
#include "includes.h"

GPUMemoryManager::GPUMemoryManager(VkPhysicalDevice device, VkDevice gpu)
{
	physicalDevice = device;
	GPU = gpu;
    allocCount = 0;
    gpuMemoryUsed = 0;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

GPUMemoryManager::~GPUMemoryManager()
{
    ReleaseAll();
}

std::shared_ptr<GPUMemoryAllocation> GPUMemoryManager::AllocateGPUMemory(VkMemoryRequirements requiredAlloc, VkMemoryPropertyFlags memoryPropertyFlags)
{
    THREAD_LOCK(lock);

    auto gpu_mem = std::make_shared<GPUMemoryAllocation>();
    gpu_mem->allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    gpu_mem->allocInfo.allocationSize = requiredAlloc.size;
    gpu_mem->allocInfo.memoryTypeIndex = FindCompatibleGPUMemoryType(requiredAlloc.memoryTypeBits, memoryPropertyFlags);
    gpu_mem->allocID = allocCount;
    allocCount++;

    VULKAN_CALL_ERROR(vkAllocateMemory(GPU, &gpu_mem->allocInfo, nullptr, &gpu_mem->handle), "Failed to allocate GPU memory!");

    gpuMemoryUsed += requiredAlloc.size;
    allocationList.push_back(gpu_mem);

    return gpu_mem;
}

void GPUMemoryManager::ReleaseGPUMemory(uint32_t allocID)
{
    THREAD_LOCK(lock);

    for (int i = 0; i < allocationList.size(); ++i)
    {
        if (allocationList[i]->allocID == allocID)
        {
            vkFreeMemory(GPU,allocationList[i]->handle,nullptr);
            allocationList.erase(allocationList.begin() + i);
        }
    }
}

void GPUMemoryManager::ReleaseAll()
{
    for (int i = 0; i < allocationList.size(); ++i)
    {
        ReleaseGPUMemory(allocationList[i]->allocID);
    }
}

uint32_t GPUMemoryManager::FindCompatibleGPUMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & memProperties) == memProperties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable GPU memory type!");
}

GPUMemoryAllocation::GPUMemoryAllocation()
{
    this->allocID = 0;
    this->allocInfo = {};
    this->handle = 0;
}

GPUMemoryAllocation::~GPUMemoryAllocation()
{

}