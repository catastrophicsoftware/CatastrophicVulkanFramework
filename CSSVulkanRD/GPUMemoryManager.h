#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>

struct GPUMemoryAllocation
{
	VkDeviceMemory       gpuMemory;
	VkMemoryAllocateInfo allocInfo;
	uint32_t allocID;

	GPUMemoryAllocation();
	~GPUMemoryAllocation();
};

class GraphicsDevice;

class GPUMemoryManager
{
public:
	GPUMemoryManager(VkPhysicalDevice device, VkDevice gpu);
	~GPUMemoryManager();

	std::shared_ptr<GPUMemoryAllocation> AllocateGPUMemory(VkMemoryRequirements requiredAlloc, VkMemoryPropertyFlags memoryPropertyFlags);
	void ReleaseGPUMemory(uint32_t allocID);
	void ReleaseAll();
private:
	std::vector<std::shared_ptr<GPUMemoryAllocation>> allocationList;
	size_t gpuMemoryUsed;
	size_t allocCount;

	std::mutex lock;

	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkPhysicalDevice physicalDevice;
	VkDevice GPU;

	uint32_t FindCompatibleGPUMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);
};