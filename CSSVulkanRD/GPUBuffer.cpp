#include "GPUBuffer.h"
#include "GraphicsDevice.h"
#include "DeviceContext.h"

void GPUBuffer::Create(size_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode,bool dynamic, bool gpuAllocate)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	if (!dynamic) bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; //not dynamic resource, needs to be copy destination to populate
	bufferInfo.sharingMode = sharingMode;

	description = bufferInfo;
	this->dynamic = dynamic;

	AllocateGPUMemory();
}

GPUBuffer::GPUBuffer(GraphicsDevice* pDevice) : GPUResource(pDevice)
{
	GPU = pDevice->GetGPU();
	this->pDevice = pDevice;
	mapped = false;
	mappable = false;
	dynamic = false;
	gpuAllocInfo = {};
	gpuStagingAllocInfo = {};

	pAllocator = pDevice->GetMainGPUMemoryAllocator();
}

GPUBuffer::~GPUBuffer()
{
	Destroy();
}

void GPUBuffer::Destroy()
{
	vkDestroyBuffer(GPU, buffer, nullptr);
	pAllocator->VMA_FreeMemory(gpuMemory);

	if (stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(GPU, stagingBuffer, nullptr);
		pAllocator->VMA_FreeMemory(gpuStagingResourceMemory);
	}
}

void GPUBuffer::Update(void* pData)
{
	if (dynamic)
	{
		void* pGPUMem = Map();
		memcpy(pGPUMem, pData, (size_t)description.size);

		UnMap();
	}
	else
	{
		void* pStagingGPUMem = pDevice->GetMainGPUMemoryAllocator()->VMA_MapMemory(gpuStagingResourceMemory);
		memcpy(pStagingGPUMem, pData, description.size);
		pAllocator->VMA_UnmapMemory(gpuStagingResourceMemory);

		auto cmdBuf = pDevice->TransferContext->GetCommandBuffer(true);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = description.size;
		vkCmdCopyBuffer(cmdBuf->handle, stagingBuffer, buffer, 1, &copyRegion);

		vkEndCommandBuffer(cmdBuf->handle);

		pDevice->TransferContext->Submit(cmdBuf);
	}
}

//void* GPUBuffer::Map()
//{
//	if (!mapped && mappable)
//	{
//		mapped = true;
//		return pAllocator->VMA_MapMemory(gpuMemory);
//	}
//	return nullptr;
//}
//
//void GPUBuffer::UnMap()
//{
//	if (mapped)
//	{
//		pAllocator->VMA_UnmapMemory(gpuMemory);
//		mapped = false;
//	}
//}

VkBuffer GPUBuffer::GetBuffer() const
{
	return buffer;
}

void GPUBuffer::AllocateGPUMemory()
{
	if (!gpuMemoryAllocated)
	{
		VmaMemoryUsage memFlags;

		if (dynamic)
		{
			memFlags = VMA_MEMORY_USAGE_CPU_TO_GPU;
			mappable = true;
		}
		else
		{
			memFlags = VMA_MEMORY_USAGE_GPU_ONLY;
			mappable = false;
		}

		gpuAllocInfo.usage = memFlags;
		buffer = pAllocator->AllocateGPUBuffer(description, &gpuAllocInfo, &gpuMemory);


		if (!dynamic)
		{
			createStagingBuffer();
		}

		gpuMemoryAllocated = true;
	}
}

bool GPUBuffer::IsDynamic() const
{
	return dynamic;
}

void GPUBuffer::createStagingBuffer()
{
	VkBufferCreateInfo stagingInfo = description;
	stagingInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; //staging buffer will be a transfer source

	gpuStagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingBuffer = pAllocator->AllocateGPUBuffer(stagingInfo, &gpuStagingAllocInfo, &gpuStagingResourceMemory);
}


DynamicPerFrameGPUBuffer::DynamicPerFrameGPUBuffer(GraphicsDevice* pDevice)
{
	this->pDevice = pDevice;
}

DynamicPerFrameGPUBuffer::~DynamicPerFrameGPUBuffer()
{
	for (int i = 0; i < buffers.size(); ++i)
	{
		buffers[i]->GPUBuffer->Destroy();
		buffers[i]->pBufferOpFence = nullptr;
	}
}

uint32_t DynamicPerFrameGPUBuffer::GetBufferCount() const
{
	return count;
}

void DynamicPerFrameGPUBuffer::Create(uint32_t bufferCount, uint32_t bufferSize, VkBufferUsageFlagBits bufferUsage)
{
	for (int i = 0; i < bufferCount; ++i)
	{
		auto buffer = std::make_shared<GPUBufferContainer>();
		buffer->pBufferOpFence = nullptr;

		buffer->GPUBuffer = new GPUBuffer(pDevice);
		buffer->GPUBuffer->Create(bufferSize, bufferUsage, VK_SHARING_MODE_EXCLUSIVE, true);

		buffers.push_back(buffer);
	}
}

std::shared_ptr<GPUBufferContainer> DynamicPerFrameGPUBuffer::GetBuffer(uint32_t index) const
{
	assert(index <= (buffers.size() - 1));
	
	return buffers[index];
}