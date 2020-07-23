#pragma once
#include "includes.h"
#include "memory-allocator/src/vk_mem_alloc.h"

class GraphicsDevice;
class GPUMemoryManager;

class GPUResource
{
public:
	GPUResource(GraphicsDevice* pDevice);
	~GPUResource();

	virtual void Destroy();
	virtual void AllocateGPUMemory() = 0;
	virtual void Update(void* pData) = 0; //copy based update operation

	void* Map();
	void  UnMap();
protected:
	GraphicsDevice* pDevice;
	shared_ptr<GPUMemoryManager> pAllocator;

	VkDevice GPU;
	VkPhysicalDevice physicalDevice;
	VkMemoryRequirements memoryRequirements;

	VkDeviceSize size;

	VmaAllocation     gpuMemory;
	VmaAllocation     gpuStagingResourceMemory;
	VmaAllocationCreateInfo gpuAllocInfo;
	VmaAllocationCreateInfo gpuStagingAllocInfo;

	bool gpuMemoryAllocated;
	bool mapped;
	bool mappable;

	bool stagingResourceExists;
};