#include "RenderPass.h"
#include "GraphicsDevice.h"
#include "DeviceContext.h"

RenderPass::RenderPass(GraphicsDevice* pDevice)
{
	this->pGraphics = pDevice;
}

RenderPass::~RenderPass()
{
}

void RenderPass::Create(VkFormat colorAttachmentFormat, VkSampleCountFlagBits colorSampleCount, VkAttachmentLoadOp colorLoadOp, VkAttachmentStoreOp colorStoreOp)
{
	colorAttachmentDesc.format = colorAttachmentFormat;
	colorAttachmentDesc.samples = colorSampleCount;
	colorAttachmentDesc.loadOp = colorLoadOp;
	colorAttachmentDesc.storeOp = colorStoreOp;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentReference;
	

	renderPassDesc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassDesc.attachmentCount = 1;
	renderPassDesc.pAttachments = &colorAttachmentDesc;
	renderPassDesc.subpassCount = 1;
	renderPassDesc.pSubpasses = &subpassDesc;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassDesc.dependencyCount = 1;
	renderPassDesc.pDependencies = &dependency;

	VULKAN_CALL_ERROR(vkCreateRenderPass(pGraphics->GetGPU(), &renderPassDesc, nullptr, &handle), "failed to create render pass");
}

void RenderPass::EndRenderPass(bool endCommandBuffer)
{
	assert(renderPassCommandBuffer != nullptr);

	vkCmdEndRenderPass(renderPassCommandBuffer->handle);
	renderPassInProgress = false;

	if (endCommandBuffer)
	{
		renderPassCommandBuffer->End();
	}

	renderPassCommandBuffer = nullptr;
}

void RenderPass::Destroy()
{

}

VkRenderPass RenderPass::GetRenderPassHandle() const
{
	return handle;
}

void RenderPass::BeginRenderPass(VkFramebuffer renderTarget, CommandBuffer* targetGPUCommandBuffer, VkRect2D renderArea)
{
	VkRenderPassBeginInfo begin{};
	begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin.renderPass = handle;
	begin.framebuffer = renderTarget;
	begin.renderArea = renderArea;
	begin.clearValueCount = 1;
	
	VkClearColorValue clearColor = { 50.0f / 256.0f, 100.0f / 255, 200.0f / 255.0f, 1.0f };
	VkClearValue clearValue = {};
	clearValue.color = clearColor;

	begin.pClearValues = &clearValue;

	vkCmdBeginRenderPass(targetGPUCommandBuffer->handle, &begin, VK_SUBPASS_CONTENTS_INLINE);
	renderPassCommandBuffer = targetGPUCommandBuffer; //store reference to command buffer used to begin render pass
	//command buffer used to begin/end the pass must be the same or everything will go to hell in a handbasket
}