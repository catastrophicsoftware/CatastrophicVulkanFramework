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

	virtual void* Map() override;
	virtual void UnMap() override;
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

	//GPUMemoryAllocation* gpuMemory;
	//GPUMemoryAllocation* stagingMem;

	VmaAllocation gpuMemory;
	VmaAllocation gpuStagingMemory;
	VmaAllocationCreateInfo allocInfo;
	VmaAllocationCreateInfo stagingAllocInfo;
};


struct GPUBufferContainer
{
	GPUBuffer* GPUBuffer;
	VkFence* pBufferOpFence; //pointer to the current fence associated
	//with the gpu operation that uses this buffer
};


class DynamicPerFrameGPUBuffer
{
public:
	DynamicPerFrameGPUBuffer(GraphicsDevice* pDevice);
	~DynamicPerFrameGPUBuffer();

	uint32_t GetBufferCount() const;

	void Create(uint32_t bufferCount, uint32_t bufferSize, VkBufferUsageFlagBits bufferUsage);

	std::shared_ptr<GPUBufferContainer> GetBuffer(uint32_t index) const;
private:
	GraphicsDevice* pDevice;
	std::vector<std::shared_ptr<GPUBufferContainer>> buffers;
	uint32_t count;
};