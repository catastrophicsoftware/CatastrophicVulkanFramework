#include "GPUBuffer.h"
#include "GraphicsDevice.h"
#include "DeviceContext.h"

void GPUBuffer::Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode,bool dynamic, bool gpuAllocate)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	if (!dynamic) bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; //not dynamic resource, needs to be copy destination to populate
	bufferInfo.sharingMode = sharingMode;
	description = bufferInfo;

	this->dynamic = dynamic;

	VULKAN_CALL(vkCreateBuffer(GPU, &bufferInfo, nullptr, &buffer));

	vkGetBufferMemoryRequirements(GPU, buffer, &memoryRequirements);

	if (gpuAllocate)
		AllocateGPUMemory();
}

GPUBuffer::GPUBuffer(GraphicsDevice* pDevice) : GPUResource(pDevice)
{
	GPU = pDevice->GetGPU();
	this->pDevice = pDevice;
	mapped = false;
	mappable = false;
	dynamic = false;
}

GPUBuffer::~GPUBuffer()
{
	Destroy();
}

void GPUBuffer::Destroy()
{
	vkDestroyBuffer(GPU, buffer, nullptr);
	ReleaseGPUMemory();

	if (stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(GPU, stagingBuffer, nullptr);
		pDevice->GetMainGPUMemoryAllocator()->ReleaseGPUMemory(stagingMem->allocID);
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
		void* pStagingMem = nullptr;
		VULKAN_CALL_ERROR(vkMapMemory(GPU, stagingMem->handle, 0, description.size, 0, &pStagingMem), "failed to map staging buffer");
		memcpy(pStagingMem, pData, description.size);
		vkUnmapMemory(GPU, stagingMem->handle);

		auto cmdBuf = pDevice->TransferContext->GetCommandBuffer(true);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = description.size;
		vkCmdCopyBuffer(cmdBuf->handle, stagingBuffer, buffer, 1, &copyRegion);

		vkEndCommandBuffer(cmdBuf->handle);

		pDevice->TransferContext->SubmitCommandBuffer(cmdBuf, true);
		int x = 0;
	}
}

void* GPUBuffer::Map()
{
	if (!mapped && mappable)
	{
		void* pGPUMemoryRegion = nullptr;
		VULKAN_CALL(vkMapMemory(GPU,gpuMemory->handle, 0, description.size, 0, &pGPUMemoryRegion));
		mapped = true;
		return pGPUMemoryRegion;
	}
	return nullptr;
}

void* GPUBuffer::Map(VkDeviceSize offset, VkDeviceSize size)
{
	if (!mapped && mappable)
	{
		void* pGPUMemoryRegion = nullptr;
		VULKAN_CALL(vkMapMemory(GPU, gpuMemory->handle, offset, size,0, &pGPUMemoryRegion));
		mapped = true;
		return pGPUMemoryRegion;
	}
	return nullptr;
}

void GPUBuffer::UnMap()
{
	if (mapped)
	{
		vkUnmapMemory(GPU, gpuMemory->handle);
		mapped = false;
	}
}

VkBuffer GPUBuffer::GetBuffer() const
{
	return buffer;
}

void GPUBuffer::AllocateGPUMemory()
{
	if (!gpuMemoryAllocated)
	{
		VkMemoryPropertyFlags memFlags;
		if (dynamic)
		{
			memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; //mappable by host
			mappable = true;
		}
		else
		{
			memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		gpuMemory = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(memoryRequirements, memFlags);
		VULKAN_CALL(vkBindBufferMemory(GPU, buffer, gpuMemory->handle, gpuMemory->offset));

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
	stagingInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VULKAN_CALL_ERROR(vkCreateBuffer(GPU, &stagingInfo, nullptr, &stagingBuffer), "failed to create staging buffer");
	VkMemoryRequirements allocReq = {};
	vkGetBufferMemoryRequirements(GPU, stagingBuffer, &allocReq);
	stagingMem = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(allocReq, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VULKAN_CALL_ERROR(vkBindBufferMemory(GPU, stagingBuffer, stagingMem->handle, 0), "failed to bind staging buffer gpu memory");
}

void GPUBuffer::ReleaseGPUMemory()
{
	if (gpuMemoryAllocated)
	{
		pDevice->GetMainGPUMemoryAllocator()->ReleaseGPUMemory(gpuMemory->allocID);

		gpuMemoryAllocated = false;
	}
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