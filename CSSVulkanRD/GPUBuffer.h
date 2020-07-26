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

	void Create(size_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode,bool dynamic=false, bool gpuAllocate=true);

	virtual void Destroy() override;
	virtual void Update(void* pData) override;

	VkBuffer GetBuffer() const;

	void AllocateGPUMemory();

	bool IsDynamic() const;
private:
	VkBufferCreateInfo   description;
	VkBuffer buffer;

	VkBuffer stagingBuffer;
	void createStagingBuffer();

	GraphicsDevice* pDevice;
	std::shared_ptr<GPUMemoryManager> pAllocator;

	bool dynamic;
};