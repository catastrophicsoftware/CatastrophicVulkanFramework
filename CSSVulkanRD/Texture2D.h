#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "GPUResource.h"
#include "GPUMemoryManager.h"

class GraphicsDevice;

class Texture2D : public GPUResource
{
public:
	Texture2D(GraphicsDevice* pDevice);
	~Texture2D();

	void Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags, bool allocateGPUMemory=true);

	virtual void Destroy() override;
private:
	VkImage texture;
	VkImageCreateInfo desc;
	std::shared_ptr<GPUMemoryAllocation> textureMem;

	uint32_t width;
	uint32_t height;
	VkFormat format;
};