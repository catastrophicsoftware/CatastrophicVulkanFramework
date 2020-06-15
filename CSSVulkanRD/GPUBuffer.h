#pragma once
#include "includes.h"

class GraphicsDevice;

class GPUBuffer
{
public:
	GPUBuffer(GraphicsDevice* pDevice);
	~GPUBuffer();

	void Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode, bool gpuAllocate=true);
	void Destroy();

	void AllocateGPUMemory();

	void FillBuffer(void* pData);

	void* Map();
	void Unmap();

	VkBuffer GetBuffer() const;
private:
	VkBuffer             buffer;
	VkBufferCreateInfo   bufferDescription;
	VkMemoryRequirements memoryRequirements;
	VkDeviceMemory       bufferGPUMemoryHandle;
	VkDevice GPU;

	GraphicsDevice* pDevice;

	bool gpuMemoryAllocated;
	bool mapped;
};