#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>
#include "memory-allocator/src/vk_mem_alloc.h"

struct UsedRange
{
	uint32_t start;
	uint32_t size;
};

struct GPUMemoryAllocation
{
	VkDeviceMemory       handle;
	VkMemoryAllocateInfo handleAllocInfo;
	VkMemoryPropertyFlags memFlags;
	uint32_t allocID;
	uint32_t offset;

	GPUMemoryAllocation(VkDeviceMemory handle,VkMemoryPropertyFlags memFlags, uint32_t allocID, uint32_t offset);
	GPUMemoryAllocation();
	~GPUMemoryAllocation();
};

struct GPUMemoryPool
{
	VkDeviceMemory          handle;
	VkMemoryAllocateInfo    handleAllocInfo;
	VkMemoryPropertyFlags   memFlags;
	uint32_t                  totalSize;
	std::vector<UsedRange*> usedRegions;
	uint32_t poolID;
};

class GraphicsDevice;


class GPUMemoryManager
{
public:
	GPUMemoryManager(VkInstance instance, VkPhysicalDevice device, VkDevice gpu);
	~GPUMemoryManager();

	GPUMemoryAllocation* AllocateGPUMemory(VkMemoryRequirements requiredAlloc, VkMemoryPropertyFlags memoryPropertyFlags);
	GPUMemoryAllocation* PoolAllocateGPUMemory(VkMemoryRequirements allocReq, VkMemoryPropertyFlags memoryPropertyFlags);

	VkBuffer AllocateGPUBuffer(VkBufferCreateInfo createInfo, const VmaAllocationCreateInfo* allocInfo, VmaAllocation* pAlloc);

	void ReleaseGPUMemory(uint32_t allocID);
	void ReleaseAll();

	void InitializeVMA(VkPhysicalDevice phsycaiDevice, VkDevice GPU, VkInstance instance);

	void* VMA_MapMemory(VmaAllocation allocation);
	void VMA_UnmapMemory(VmaAllocation allocation);
	void VMA_FreeMemory(VmaAllocation allocation);
private:
	std::vector<GPUMemoryAllocation*> allocationList;
	uint32_t gpuMemoryUsed;
	uint32_t allocCount;
	uint32_t poolCount;

	uint32_t getAllocID();
	uint32_t getPoolID();

	std::vector<GPUMemoryPool*> memoryPools;
	GPUMemoryPool* createMemoryPool(uint32_t size, VkMemoryPropertyFlags memoryProperties, VkMemoryRequirements allocReq);

	std::mutex lock;

	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkPhysicalDevice physicalDevice;
	VkDevice GPU;

	uint32_t FindCompatibleGPUMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);

	VmaAllocator memoryAllocator;
};