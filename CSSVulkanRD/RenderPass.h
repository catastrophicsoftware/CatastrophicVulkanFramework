#pragma once
#include "includes.h"

class GraphicsDevice;
struct CommandBuffer;

class RenderPass
{
public:
	RenderPass(GraphicsDevice* pDevice);
	~RenderPass();

	void Create(VkFormat colorAttachmentFormat, VkSampleCountFlagBits colorSampleCount, VkAttachmentLoadOp colorLoadOp, VkAttachmentStoreOp colorStoreOp);
	void BeginRenderPass(VkFramebuffer renderTarget,CommandBuffer* targetGPUCommandBuffer, VkRect2D renderArea);
	void EndRenderPass(bool endCommandBuffer=false);
	void Destroy();

	VkRenderPass GetRenderPassHandle() const;
private:
	GraphicsDevice* pGraphics;

	VkAttachmentDescription colorAttachmentDesc;
	VkAttachmentReference colorAttachmentReference;
	VkSubpassDependency subpass;
	VkRenderPassCreateInfo renderPassDesc;
	VkSubpassDescription subpassDesc;

	VkRenderPass handle;

	bool renderPassInProgress;
	CommandBuffer* renderPassCommandBuffer;
};