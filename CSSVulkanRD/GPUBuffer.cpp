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
	Destroy();
}

void GPUBuffer::Destroy()
{
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
		VULKAN_CALL(vkMapMemory(GPU,gpuMemoryHandle, 0, description.size, 0, &pGPUMemoryRegion));
		mapped = true;
		return pGPUMemoryRegion;
	}
	return nullptr;
}

void GPUBuffer::Unmap()
{
	if (mapped)
	{
		vkUnmapMemory(GPU, gpuMemoryHandle);
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
		VkMemoryAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.allocationSize = memoryRequirements.size;
		alloc.memoryTypeIndex = pDevice->FindGPUMemory(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		VULKAN_CALL_ERROR(vkAllocateMemory(GPU, &alloc, nullptr, &gpuMemoryHandle),"Failed to allocate buffer gpu memory");
		VULKAN_CALL_ERROR(vkBindBufferMemory(GPU, buffer, gpuMemoryHandle, 0),"failed to bind buffer gpu memory");
		gpuMemoryAllocated = true;
	}
}

void GPUBuffer::ReleaseGPUMemory()
{
	if (gpuMemoryAllocated)
	{
		vkFreeMemory(GPU,gpuMemoryHandle,nullptr);
		gpuMemoryAllocated = false;
	}
}