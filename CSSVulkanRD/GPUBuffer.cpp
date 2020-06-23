#include "GPUBuffer.h"
#include "GraphicsDevice.h"

void GPUBuffer::Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode, bool gpuAllocate)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;
	description = bufferInfo;

	GPU = pDevice->GetGPU();

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
	mappable = true;
}

GPUBuffer::~GPUBuffer()
{
}

void GPUBuffer::Destroy()
{
	vkDestroyBuffer(GPU, buffer, nullptr);
	ReleaseGPUMemory();
}


void GPUBuffer::FillBuffer(void* pData)
{
	void* pGPUMem = Map();
	memcpy(pGPUMem, pData, (size_t)description.size);

	Unmap();
}

void* GPUBuffer::Map()
{
	if (!mapped)
	{
		void* pGPUMemoryRegion = nullptr;
		VULKAN_CALL(vkMapMemory(GPU,gpuMemory->gpuMemory, 0, description.size, 0, &pGPUMemoryRegion));
		mapped = true;
		return pGPUMemoryRegion;
	}
	return nullptr;
}

void GPUBuffer::Unmap()
{
	if (mapped)
	{
		vkUnmapMemory(GPU, gpuMemory->gpuMemory);
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
		gpuMemory = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		VULKAN_CALL(vkBindBufferMemory(GPU, buffer, gpuMemory->gpuMemory, 0));

		gpuMemoryAllocated = true;
	}
}

void GPUBuffer::ReleaseGPUMemory()
{
	if (gpuMemoryAllocated)
	{
		pDevice->GetMainGPUMemoryAllocator()->ReleaseGPUMemory(gpuMemory->allocID);

		gpuMemoryAllocated = false;
	}
}