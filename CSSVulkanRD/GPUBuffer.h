#pragma once
#include "includes.h"
#include "GPUResource.h"
#include "GPUMemoryManager.h"

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
	VkMemoryRequirements memoryRequirements;
	VkDevice GPU;

	GraphicsDevice* pDevice;

	std::shared_ptr<GPUMemoryAllocation> gpuMemory;

	bool gpuMemoryAllocated;
	bool mapped;
	bool mappable;
};