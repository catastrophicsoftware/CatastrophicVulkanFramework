#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "GPUResource.h"
#include "GPUMemoryManager.h"
#include "memory-allocator/src/vk_mem_alloc.h"

class GraphicsDevice;
class GPUBuffer;

class Texture2D : public GPUResource
{
public:
	Texture2D(GraphicsDevice* pDevice);
	~Texture2D();

	void Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags,bool createSampler=false,bool mappable=false, bool allocateGPUMemory=true);
	void CreateFromFile(const char* textureFilePath, bool createImageSampler=false);

	virtual void Destroy() override;
	virtual void AllocateGPUMemory() override;
	virtual void Update(void* pData) override;
	void Update(VkBuffer srcBuffer);

	VkImage GetTexture() const;
	VkImageView GetImageView() const;
	VkSampler GetSampler() const;

	void transitionImageLayout(VkFormat format, VkImageLayout prevLayout, VkImageLayout newLayout);
private:
	VkImage texture;
	GPUBuffer* stagingBuffer;

	VkImageCreateInfo desc;

	VkImageView imageView;
	VkSampler imageSampler;

	void createSampler();
	void createImageView();

	VmaAllocation textureMem;
	VmaAllocation stagingTextureMem;

	VmaAllocationCreateInfo allocInfo;
	VmaAllocationCreateInfo stagingAllocInfo;
	void createStagingResource();
	void destroyStagingResource();

	uint32_t width;
	uint32_t height;
	uint32_t loadedDataSize;
	VkFormat format;
};