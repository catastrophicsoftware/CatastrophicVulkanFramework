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

	void Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode,bool dynamic=false, bool gpuAllocate=true);

	void FillBuffer(void* pData);

	void* Map();
	void Unmap();
	virtual void Destroy() override;

	VkBuffer GetBuffer() const;

	void ReleaseGPUMemory();
	void AllocateGPUMemory();

	bool IsDynamic() const;
private:
	VkBufferCreateInfo   description;
	VkBuffer buffer;

	VkBuffer stagingBuffer;
	void createStagingBuffer();

	GraphicsDevice* pDevice;

	bool dynamic;

	std::shared_ptr<GPUMemoryAllocation> gpuMemory;
	std::shared_ptr<GPUMemoryAllocation> stagingMem;
};