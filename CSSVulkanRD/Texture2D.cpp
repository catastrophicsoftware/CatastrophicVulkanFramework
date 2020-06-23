#include "Texture2D.h"
#include "GraphicsDevice.h"

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

void Texture2D::Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags, bool allocateGPUMemory)
{
	this->width = width;
	this->height = height;
	this->format = format;

	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.format = format;
	desc.extent.width = width;
	desc.extent.height = height;
	desc.extent.depth = 1; //2D texture only
	desc.mipLevels = 1; //mip levels other than 1 currently not supported by catastrophic engine
	desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.usage = imageUsageFlags;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //other values not supported by CATEngine

	VULKAN_CALL_ERROR(vkCreateImage(GPU, &desc, nullptr, &texture), "failed to create texture2D");
	vkGetImageMemoryRequirements(GPU, texture, &memoryRequirements);

	if (allocateGPUMemory)
	{
		textureMem = pDevice->GetMainGPUMemoryAllocator()->AllocateGPUMemory(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(GPU, texture, textureMem->gpuMemory, 0);
		gpuMemoryAllocated = true;
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