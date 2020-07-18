#include "Texture2D.h"
#include "GraphicsDevice.h"
#include "GPUBuffer.h"
#include "DeviceContext.h"

Texture2D::Texture2D(GraphicsDevice* pDevice) : GPUResource(pDevice)
{
	width = 0;
	height = 0;
	format = VK_FORMAT_UNDEFINED;
	desc = {};
}

Texture2D::~Texture2D()
{
}

void Texture2D::Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags,bool mappable, bool allocateGPUMemory)
{
	this->width = width;
	this->height = height;
	this->format = format;
	this->mappable = mappable;

	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.format = format;
	desc.extent.width = width;
	desc.extent.height = height;
	desc.extent.depth = 1; //2D texture only
	desc.mipLevels = 1; //mip levels other than 1 currently not supported by catastrophic engine
	desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.usage = imageUsageFlags;

	if (!mappable)desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //other values not supported by CATEngine

	VULKAN_CALL_ERROR(vkCreateImage(GPU, &desc, nullptr, &texture), "failed to create texture2D");
	vkGetImageMemoryRequirements(GPU, texture, &memoryRequirements);

	if (allocateGPUMemory)
	{
		AllocateGPUMemory();
	}
}

void Texture2D::Destroy()
{
	vkDestroyImage(GPU, texture, nullptr);
	if (gpuMemoryAllocated)
	{
		pDevice->GetMainGPUMemoryAllocator()->ReleaseGPUMemory(textureMem->allocID);
		gpuMemoryAllocated = false;
	}
}

void Texture2D::AllocateGPUMemory()
{
	if (mappable)
	{
		textureMem = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindImageMemory(GPU, texture, textureMem->handle, 0);
		gpuMemoryAllocated = true;
	}
	else
	{
		textureMem = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(GPU, texture, textureMem->handle, 0);

		createStagingResource();
	}
}

void Texture2D::Update(void* pData)
{
	if (mappable)
	{
		void* pMem = Map();
		memcpy(pMem, pData, memoryRequirements.size);
		UnMap();
	}
	else
	{
		void* pStagingMem = stagingBuffer->Map();
		memcpy(pStagingMem, pData, memoryRequirements.size);
		stagingBuffer->UnMap();

		transitionImageLayout(desc.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		auto xfrCmd = pDevice->GetTransferContext()->GetCommandBuffer(true);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(xfrCmd->handle, stagingBuffer->GetBuffer(), texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1, &region);
		
		vkEndCommandBuffer(xfrCmd->handle);
		pDevice->TransferContext->Submit(xfrCmd);

		transitionImageLayout(desc.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void* Texture2D::Map()
{
	if (mappable && !mapped)
	{
		void* pGPUMemoryRegion = nullptr;
		VULKAN_CALL_ERROR(vkMapMemory(GPU, textureMem->handle, 0, memoryRequirements.size, 0, &pGPUMemoryRegion), "failed to map texture2D gpu memory");
		mapped = true;
		return pGPUMemoryRegion;
	}
	return nullptr;
}

void Texture2D::UnMap()
{
	if (mappable && mapped)
	{
		vkUnmapMemory(GPU, textureMem->handle);
		mapped = false;
	}
}

void Texture2D::transitionImageLayout(VkFormat format, VkImageLayout prevLayout, VkImageLayout newLayout)
{
	auto cmdBuf = pDevice->ImmediateContext->GetCommandBuffer(true); //TODO: implement a way to set which context will be used to generate and submit these command lists

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = prevLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (prevLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (prevLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		cmdBuf->handle,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	pDevice->ImmediateContext->Submit(cmdBuf);
}

void Texture2D::createStagingResource()
{
	stagingBuffer = new GPUBuffer(pDevice);
	stagingBuffer->Create(memoryRequirements.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, true);
}

void Texture2D::destroyStagingResource()
{
	stagingBuffer->Destroy();
}