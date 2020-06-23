#include "GPUBuffer.h"
#include "GraphicsDevice.h"

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
}

void GPUBuffer::Destroy()
{
	vkDestroyBuffer(GPU, buffer, nullptr);
	ReleaseGPUMemory();
}


void GPUBuffer::FillBuffer(void* pData)
{
	if (dynamic)
	{
		void* pGPUMem = Map();
		memcpy(pGPUMem, pData, (size_t)description.size);

		Unmap();
	}
	else
	{
		void* pStagingMem = nullptr;
		VULKAN_CALL_ERROR(vkMapMemory(GPU, stagingMem->gpuMemory, 0, description.size, 0, &pStagingMem), "failed to map staging buffer");
		memcpy(pStagingMem, pData, description.size);
		vkUnmapMemory(GPU, stagingMem->gpuMemory);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pDevice->GetPrimaryCommandPool(); //streamline this, submit on background thread / pool
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuf;
		VULKAN_CALL_ERROR(vkAllocateCommandBuffers(GPU, &allocInfo, &cmdBuf), "failed to allocate staging copy command buffer");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = description.size;
		vkCmdCopyBuffer(cmdBuf, stagingBuffer, buffer, 1, &copyRegion);

		vkEndCommandBuffer(cmdBuf);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf;

		pDevice->PrimaryGPUQueueSubmit(submitInfo, true);

		vkFreeCommandBuffers(GPU, pDevice->GetPrimaryCommandPool(), 1, &cmdBuf);
	}
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
		VULKAN_CALL(vkBindBufferMemory(GPU, buffer, gpuMemory->gpuMemory, 0));

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
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VULKAN_CALL_ERROR(vkCreateBuffer(GPU, &stagingInfo, nullptr, &stagingBuffer), "failed to create staging buffer");
	VkMemoryRequirements allocReq = {};
	vkGetBufferMemoryRequirements(GPU, stagingBuffer, &allocReq);
	stagingMem = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(allocReq, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VULKAN_CALL_ERROR(vkBindBufferMemory(GPU, stagingBuffer, stagingMem->gpuMemory, 0), "failed to bind staging buffer gpu memory");
}

void GPUBuffer::ReleaseGPUMemory()
{
	if (gpuMemoryAllocated)
	{
		pDevice->GetMainGPUMemoryAllocator()->ReleaseGPUMemory(gpuMemory->allocID);

		gpuMemoryAllocated = false;
	}
}