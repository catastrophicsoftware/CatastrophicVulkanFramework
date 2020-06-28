#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>

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
	GPUMemoryManager(VkPhysicalDevice device, VkDevice gpu);
	~GPUMemoryManager();

	GPUMemoryAllocation* AllocateGPUMemory(VkMemoryRequirements requiredAlloc, VkMemoryPropertyFlags memoryPropertyFlags);
	GPUMemoryAllocation* PoolAllocateGPUMemory(VkMemoryRequirements allocReq, VkMemoryPropertyFlags memoryPropertyFlags);


	void ReleaseGPUMemory(uint32_t allocID);
	void ReleaseAll();
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
};