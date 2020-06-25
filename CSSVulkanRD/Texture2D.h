#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "GPUResource.h"
#include "GPUMemoryManager.h"

class GraphicsDevice;
class GPUBuffer;

class Texture2D : public GPUResource
{
public:
	Texture2D(GraphicsDevice* pDevice);
	~Texture2D();

	void Create(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags imageUsageFlags,bool mappable=false, bool allocateGPUMemory=true);

	virtual void Destroy() override;
	virtual void AllocateGPUMemory() override;
	virtual void Update(void* pData) override;

	virtual void* Map() override;
	virtual void UnMap() override;
private:
	VkImage texture;
	GPUBuffer* stagingBuffer;

	VkImageCreateInfo desc;

	void transitionImageLayout(VkFormat format, VkImageLayout prevLayout, VkImageLayout newLayout);

	std::shared_ptr<GPUMemoryAllocation> textureMem;
	std::shared_ptr<GPUMemoryAllocation> stagingMem;
	void createStagingResource();
	void destroyStagingResource();

	uint32_t width;
	uint32_t height;
	VkFormat format;
};