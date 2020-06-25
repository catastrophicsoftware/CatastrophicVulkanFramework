#pragma once
#include "includes.h"

class GraphicsDevice;

class GPUResource
{
public:
	GPUResource(GraphicsDevice* pDevice);
	~GPUResource();

	virtual void Destroy() = 0;
	virtual void AllocateGPUMemory() = 0;
	virtual void Update(void* pData) = 0; //copy based update operation (in theory. You do you)

	virtual void* Map() = 0;
	virtual void UnMap() = 0;
protected:
	GraphicsDevice* pDevice;
	VkDevice GPU;
	VkPhysicalDevice physicalDevice;
	VkMemoryRequirements memoryRequirements;

	bool gpuMemoryAllocated;
	bool mapped;
	bool mappable;
};