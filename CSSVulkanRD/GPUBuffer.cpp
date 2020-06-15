#include "GPUBuffer.h"
#include "GraphicsDevice.h"

void GPUBuffer::Create(size_t size, VkBufferUsageFlagBits usage, VkSharingMode sharingMode, bool gpuAllocate)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;
	bufferDescription = bufferInfo;

	VULKAN_CALL(vkCreateBuffer(GPU, &bufferInfo, nullptr, &buffer));

	vkGetBufferMemoryRequirements(GPU, buffer, &memoryRequirements);

	if (gpuAllocate)
		AllocateGPUMemory();
}

GPUBuffer::GPUBuffer(GraphicsDevice* pDevice)
{
	this->pDevice = pDevice;
	GPU = pDevice->GetGPU();
	mapped = false;
}

GPUBuffer::~GPUBuffer()
{
	Destroy();
}

void GPUBuffer::Destroy()
{
	vkDestroyBuffer(pDevice->GetGPU(), buffer, nullptr);

	if (gpuMemoryAllocated)
		vkFreeMemory(GPU, bufferGPUMemoryHandle, nullptr);
}

void GPUBuffer::AllocateGPUMemory()
{
	if (!gpuMemoryAllocated)
	{
		VkMemoryAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.memoryTypeIndex = pDevice->FindGPUMemory(memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		alloc.allocationSize = memoryRequirements.size;

		VULKAN_CALL(vkAllocateMemory(GPU, &alloc, nullptr, &bufferGPUMemoryHandle));
		vkBindBufferMemory(GPU, buffer, bufferGPUMemoryHandle, 0);

		gpuMemoryAllocated = true;
	}
}

void GPUBuffer::FillBuffer(void* pData)
{
	void* pGPUMem = Map();
	memcpy(pGPUMem, pData, (size_t)bufferDescription.size);

	Unmap();
}

void* GPUBuffer::Map()
{
	if (!mapped)
	{
		void* pGPUMemoryRegion = nullptr;
		VULKAN_CALL(vkMapMemory(GPU, bufferGPUMemoryHandle, 0, bufferDescription.size, 0, &pGPUMemoryRegion));
		mapped = true;
		return pGPUMemoryRegion;
	}
	else
		throw std::runtime_error("Buffer already mapped!");
}

void GPUBuffer::Unmap()
{
	if (mapped)
	{
		vkUnmapMemory(GPU, bufferGPUMemoryHandle);
		mapped = false;
	}
}

VkBuffer GPUBuffer::GetBuffer() const
{
	return buffer;
}
