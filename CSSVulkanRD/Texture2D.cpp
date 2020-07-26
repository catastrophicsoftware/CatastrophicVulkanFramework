#include "Texture2D.h"
#include "GraphicsDevice.h"
#include "GPUBuffer.h"
#include "DeviceContext.h"
#include "stb_image.h"

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

void Texture2D::Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags, bool createImageSampler, bool mappable, bool allocateGPUMemory)
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
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.imageType = VK_IMAGE_TYPE_2D;
	desc.arrayLayers = 1;

	if (!mappable)desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //other values not supported by CATEngine

	VULKAN_CALL_ERROR(vkCreateImage(GPU, &desc, nullptr, &texture), "failed to create texture2D");
	vkGetImageMemoryRequirements(GPU, texture, &memoryRequirements);

	AllocateGPUMemory();

	if (createImageSampler)
	{
		createImageView();
		createSampler();
	}
}

void Texture2D::CreateFromFile(const char* textureFilePath, bool createImageSampler)
{
	int x, y, channels;
	unsigned char* pixels = stbi_load(textureFilePath, &x, &y, &channels, STBI_rgb_alpha);
	loadedDataSize = x * y * 4;
	width = x;
	height = y;

	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;


	format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.format = format; //TODO: figure out how the fuck these stbi channels map to vulkan image formats


	desc.extent.width = width;
	desc.extent.height = height;
	desc.extent.depth = 1; //2D texture only
	desc.mipLevels = 1; //mip levels other than 1 currently not supported by catastrophic engine
	desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.imageType = VK_IMAGE_TYPE_2D;
	desc.arrayLayers = 1;

	if (!mappable)desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //other values not supported by CATEngine

	VULKAN_CALL_ERROR(vkCreateImage(GPU, &desc, nullptr, &texture), "failed to create texture2D");
	vkGetImageMemoryRequirements(GPU, texture, &memoryRequirements);

	AllocateGPUMemory();

	//assumption for now is that textures loaded from file are not dynamic and will reside in GPU-physical memory

	Update(pixels);

	if (createImageSampler)
	{
		createImageView();
		createSampler();
	}
}

void Texture2D::Destroy()
{
	vkDestroyImage(GPU, texture, nullptr);
	if (gpuMemoryAllocated)
	{
		pDevice->GetMainGPUMemoryAllocator()->VMA_FreeMemory(textureMem);
		gpuMemoryAllocated = false;

		vkDestroyImageView(GPU, imageView, nullptr);
		vkDestroySampler(GPU, imageSampler, nullptr);

		if (!mappable)
			destroyStagingResource();
	}
}

void Texture2D::AllocateGPUMemory()
{
	VmaMemoryUsage memFlags;

	if (mappable)
	{
		memFlags = VMA_MEMORY_USAGE_CPU_TO_GPU;
	}
	else
	{
		memFlags = VMA_MEMORY_USAGE_GPU_ONLY;
		createStagingResource();
	}

	allocInfo.usage = memFlags;

	texture = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUImage(desc, &allocInfo, &textureMem);
	gpuMemoryAllocated = true;
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
		memcpy(pStagingMem, pData, static_cast<size_t>(loadedDataSize));
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

VkImage Texture2D::GetTexture() const
{
	return texture;
}

VkImageView Texture2D::GetImageView() const
{
	return imageView;
}

VkSampler Texture2D::GetSampler() const
{
	return imageSampler;
}

void Texture2D::createSampler()
{
	VkSamplerCreateInfo sci{};
	sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sci.magFilter = VK_FILTER_LINEAR;
	sci.minFilter = VK_FILTER_LINEAR;
	sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sci.anisotropyEnable = VK_TRUE;
	sci.maxAnisotropy = 16.0f;
	sci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sci.unnormalizedCoordinates = VK_FALSE;
	sci.compareEnable = VK_FALSE;
	sci.compareOp = VK_COMPARE_OP_ALWAYS;
	sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sci.mipLodBias = 0.0f;
	sci.minLod = 0.0f;
	sci.maxLod = 0.0f;

	VULKAN_CALL_ERROR(vkCreateSampler(GPU, &sci, nullptr, &imageSampler), "failed to create image sampler!");
}

void Texture2D::createImageView()
{
	VkImageViewCreateInfo ivci{};
	ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ivci.image = texture;
	ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ivci.format = desc.format;
	ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ivci.subresourceRange.baseMipLevel = 0;
	ivci.subresourceRange.levelCount = 1;
	ivci.subresourceRange.baseArrayLayer = 0;
	ivci.subresourceRange.layerCount = 1;
	
	VULKAN_CALL_ERROR(vkCreateImageView(GPU, &ivci, nullptr, &imageView), "failed to create image view!");
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
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

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

	vkEndCommandBuffer(cmdBuf->handle);

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