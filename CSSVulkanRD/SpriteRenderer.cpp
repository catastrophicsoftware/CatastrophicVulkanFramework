#include "SpriteRenderer.h"
#include "GPUBuffer.h"
#include "Shader.h"
#include "DeviceContext.h"
#include <glm/gtc/matrix_transform.hpp>
#include "VertexTypes.h"
#include "GraphicsDevice.h"
#include "PipelineState.h"
#include "RenderPass.h"
#include "Texture2D.h"

VertexPositionTexture spriteVertices[] =
{
	{{-0.5f,0.5f},{0.0f,0.0f}},
	{{0.5f,0.5f},{1.0f,0.0f}},
	{{0.5f,-0.5f},{1.0f,1.0f}},
	{{-0.5f,-0.5f},{0.0f,1.0f}}
};

uint16_t spriteIndices[] = { 0,1,2, 2,3,0 };

SpriteRenderer::SpriteRenderer(GraphicsDevice* pDevice)
{
	spriteRenderPassActive = false;
    spritePipelineBound = false;
    this->pDevice = pDevice;
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::Initialize(int windowWidth, int windowHeight, int swapchainCount)
{
	width = windowWidth;
	height = windowHeight;
	swapchainFramebufferCount = swapchainCount;

    createPipelineState();
	createBuffers();
}

void SpriteRenderer::RenderSprite(Texture2D* Sprite, glm::vec2 position, float rotation)
{
    if (!spritePipelineBound)
    {
        vkCmdBindPipeline(spriteCMD->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipeline());
        spritePipelineBound = true;
    }

    spriteTransform = glm::translate(spriteTransform, glm::vec3(position, 1));

    memcpy(pTransformBufferGPUMemory, &spriteTransform, sizeof(glm::mat4));

    spritePipeline->UpdateUniformBufferDescriptor(frameIndex, 1, cbSpriteTransform->GetBuffer(), 0, VK_WHOLE_SIZE);
    
    spritePipeline->UpdateCombinedImageDescriptor(frameIndex, 2, Sprite->GetImageView(), Sprite->GetSampler()); //TODO: give the sprite renderer it's own sampler
    
    VkDescriptorSet spriteDescriptorSet = spritePipeline->GetDescriptorSet(frameIndex);

    vkCmdBindDescriptorSets(spriteCMD->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipelineLayout(), 0, 1, &spriteDescriptorSet, 0, nullptr);
    VkBuffer vb = vertexBuffer->GetBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(spriteCMD->handle, 0, 1,&vb, &offset);
    vkCmdBindIndexBuffer(spriteCMD->handle, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(spriteCMD->handle, 6, 1, 0, 0, 0);
}

void SpriteRenderer::RenderSprite(CommandBuffer* gpuCommandBuffer, Texture2D* Sprite, glm::vec2 position, float rotation)
{
    vkCmdBindPipeline(gpuCommandBuffer->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipeline());

    spriteTransform = glm::translate(spriteTransform, glm::vec3(position, 1));

    memcpy(pTransformBufferGPUMemory, &spriteTransform, sizeof(glm::mat4));

    spritePipeline->UpdateUniformBufferDescriptor(frameIndex, 1, cbSpriteTransform->GetBuffer(), 0, VK_WHOLE_SIZE);
    spritePipeline->UpdateCombinedImageDescriptor(frameIndex, 2, Sprite->GetImageView(), Sprite->GetSampler()); //TODO: give the sprite renderer it's own sampler

    VkDescriptorSet spriteDescriptorSet = spritePipeline->GetDescriptorSet(frameIndex);

    vkCmdBindDescriptorSets(gpuCommandBuffer->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipelineLayout(), 0, 1, &spriteDescriptorSet, 0, nullptr);
    VkBuffer vb = vertexBuffer->GetBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(gpuCommandBuffer->handle, 0, 1, &vb, &offset);
    vkCmdBindIndexBuffer(gpuCommandBuffer->handle, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(gpuCommandBuffer->handle, 6, 1, 0, 0, 0);
}

void SpriteRenderer::BeginSpriteRenderPass(uint32_t frameIndex)
{
	if (!spriteRenderPassActive)
	{
        this->frameIndex = frameIndex;
		spriteRenderPassActive = true;
	}
}

void SpriteRenderer::EndSpriteRenderPass(bool submit)
{
	if (spriteRenderPassActive)
	{
		//spriteCMD->End();
		if (submit)
			pDevice->ImmediateContext->Submit(spriteCMD,true); //TODO: figure out if we need to block here

		spriteRenderPassActive = false;
        spritePipelineBound = false;
	}
}

glm::mat4 SpriteRenderer::getTransform() const
{
	return cameraTransform;
}

void SpriteRenderer::RecreatePipelineState()
{
    spritePipeline->Destroy();

    createPipelineState();
}

void SpriteRenderer::updateCameraTransform()
{
}


void SpriteRenderer::createBuffers()
{
	cbProjection = new GPUBuffer(pDevice);
    cbProjection->Create(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    cameraTransform = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    cbProjection->Update(&cameraTransform);

    
    for (int i = 0; i < pDevice->GetSwapchainFramebufferCount(); ++i) //write all 3 projection transform descriptors
        spritePipeline->UpdateUniformBufferDescriptor(i, 0, cbProjection->GetBuffer(), 0, VK_WHOLE_SIZE);

	cbProjection->Update(&cameraTransform);

	cbSpriteTransform = new GPUBuffer(pDevice);
	cbSpriteTransform->Create(sizeof(glm::mat4) * swapchainFramebufferCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, true);
    pTransformBufferGPUMemory = cbSpriteTransform->Map();

	vertexBuffer = new GPUBuffer(pDevice);
	vertexBuffer->Create(sizeof(VertexPositionTexture) * 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    vertexBuffer->Update(&spriteVertices);

    indexBuffer = new GPUBuffer(pDevice);
    indexBuffer->Create(sizeof(uint16_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    indexBuffer->Update(&spriteIndices);
}

void SpriteRenderer::createPipelineState()
{
	spritePipeline = new PipelineState(pDevice->GetGPU(), pDevice->GetSwapchainFramebufferCount());

    spriteShader = new Shader(pDevice->GetGPU());
    spriteShader->LoadShader("shaders\\sprite_vs.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
    spriteShader->LoadShader("shaders\\sprite_ps.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
	
    auto swapExt = pDevice->GetSwapchainExtent();
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapExt.width;
    viewport.height = (float)swapExt.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    spritePipeline->SetViewport(viewport);

    VertexInputData vsInputData;
    vsInputData.vertexBindingDesc = VertexPositionTexture::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> vertexAttrDesc = { VertexPositionTexture::GetVertexAttributeDescriptions()[0],VertexPositionTexture::GetVertexAttributeDescriptions()[1] }; //HACK!
    vsInputData.vertexInputAttributeDescriptions = vertexAttrDesc;

    spritePipeline->SetVertexInput(vsInputData);
    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = pDevice->GetSwapchainExtent();

    spritePipeline->SetScissor(scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    spritePipeline->SetRasterizerState(rasterizer);

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    spritePipeline->SetMultisamplingState(multisampling);

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    BlendState blendState;
    blendState.blendState = colorBlending;
    blendState.colorBlendAttachment = colorBlendAttachment;

    spritePipeline->SetBlendState(blendState);
    spritePipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    spritePipeline->SetPrimitiveRestartEnable(VK_FALSE);
    spritePipeline->SetRenderPass(pDevice->GetRenderPass()->GetRenderPassHandle());
    spritePipeline->SetDescriptorPool(pDevice->ImmediateContext->GetDescriptorPool());
    spritePipeline->SetShader(spriteShader);


    VkDescriptorSetLayoutBinding spriteTransformBinding{};
    spriteTransformBinding.descriptorCount = 1;
    spriteTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    spriteTransformBinding.binding = 0;
    spriteTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding cameraTransformBinding{};
    cameraTransformBinding.descriptorCount = 1;
    cameraTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraTransformBinding.binding = 1;
    cameraTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding spriteSampler{};
    spriteSampler.binding = 2;
    spriteSampler.descriptorCount = 1;
    spriteSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    spriteSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    spritePipeline->RegisterDescriptorSetLayoutBinding(spriteTransformBinding);
    spritePipeline->RegisterDescriptorSetLayoutBinding(spriteSampler);
    spritePipeline->RegisterDescriptorSetLayoutBinding(cameraTransformBinding);

    spritePipeline->Build();
    spritePipeline->CreateDescriptorSets(); //TODO: look into having the pipeline create the descriptor sets
    //on build.
}