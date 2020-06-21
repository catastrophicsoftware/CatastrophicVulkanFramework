#pragma once
#include "includes.h"
#include "GPUResource.h"

class GraphicsDevice;

class GPUBuffer : public GPUResource
{
public:
	GPUBuffer(GraphicsDevice* pDevice);
	~GPUBuffer();

	void Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode, bool gpuAllocate=true);

	void FillBuffer(void* pData);

	void* Map();
	void Unmap();
	virtual void Destroy() override;

	VkBuffer GetBuffer() const;

	void ReleaseGPUMemory();
	void AllocateGPUMemory();
private:
	VkBufferCreateInfo   description;
	VkBuffer buffer;
	VkDeviceMemory gpuMemoryHandle;
	VkMemoryRequirements memoryRequirements;
	VkDevice GPU;

	GraphicsDevice* pDevice;

	bool gpuMemoryAllocated;
	bool mapped;
	bool mappable;
};