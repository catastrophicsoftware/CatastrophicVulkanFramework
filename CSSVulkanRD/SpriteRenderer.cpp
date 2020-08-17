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
    createSpriteSampler();
}

/*
// Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = vulkanDevice->properties.limits.minUniformBufferOffsetAlignment;
    dynamicAlignment = sizeof(glm::mat4);
    if (minUboAlignment > 0) {
        dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
}*/

void SpriteRenderer::RenderSprite(Texture2D* Sprite, glm::vec2 position, float rotation)
{
    if (!spritePipelineBound)
    {
        vkCmdBindPipeline(pActiveSpriteCMD->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipeline());
        spritePipelineBound = true;
    }

    spriteTransform = glm::translate(spriteTransform, glm::vec3(position, 1));

    size_t minUBO_Align = pDevice->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
    size_t dynamic_align = sizeof(glm::mat4);
    if (minUBO_Align > 0)
    {
        dynamic_align = (dynamic_align + minUBO_Align - 1) & ~(minUBO_Align - 1);
    }
    size_t finalOffset = dynamic_align * frameIndex;
    char* pMem = (char*)pTransformBufferGPUMemory;
    pMem = pMem + finalOffset;
    memcpy(pMem, &spriteTransform, sizeof(glm::mat4));

    int id = Sprite->GetID();

    vkCmdPushConstants(pActiveSpriteCMD->handle, spritePipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &id);
    VkBuffer vb = vertexBuffer->GetBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(pActiveSpriteCMD->handle, 0, 1,&vb, &offset);
    vkCmdBindIndexBuffer(pActiveSpriteCMD->handle, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(pActiveSpriteCMD->handle, 6, 1, 0, 0, 0);
}

void SpriteRenderer::BeginSpriteRenderPass(CommandBuffer* pGPUCommandBuffer, uint32_t frameIndex)
{
	if (!spriteRenderPassActive)
	{
        this->frameIndex = frameIndex;
        pActiveSpriteCMD = pGPUCommandBuffer;
        spriteFences.push_back(pActiveSpriteCMD->fence); //store fence

        VkDescriptorSet spriteDescriptorSet = spritePipeline->GetDescriptorSet(frameIndex);
        vkCmdBindDescriptorSets(pActiveSpriteCMD->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline->GetPipelineLayout(), 0, 1, &spriteDescriptorSet, 0, nullptr);

		spriteRenderPassActive = true;
	}
}

void SpriteRenderer::EndSpriteRenderPass(bool submit)
{
	if (spriteRenderPassActive)
	{
		//spriteCMD->End();
		//if (submit)
		//	pDevice->ImmediateContext->Submit(spriteCMD,true); //TODO: figure out if we need to block here

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

void SpriteRenderer::SetSprites(std::vector<Texture2D*> sprite_set)
{
    assert(sprite_set.size() <= SPRITE_ARRAY_SIZE);

    if (spriteFences.size() > 0) //wait for all sprite command buffers to complete before updating sprite array descriptor
    {
        VkFence* fences = spriteFences.data();
        vkWaitForFences(pDevice->GetGPU(), spriteFences.size(), fences, VK_TRUE, INFINITE);
        spriteFences.clear();
    }

    int filled_sets = sprite_set.size();
    int empty_sets = SPRITE_ARRAY_SIZE - filled_sets;

    std::vector<VkImageView> imageViews;

    int s = 0;
    for (s = 0; s < filled_sets; ++s)
    {
        imageViews.push_back(sprite_set[s]->GetImageView()); //fill populated sets
        sprite_set[s]->SetID(s);
    }
    for (s = s; s < empty_sets; ++s)
    {
        imageViews.push_back(nullTexture->GetImageView()); //fill empty sets will 1x1 "null texture"
    }


    for (int i = 0; i < pDevice->GetSwapchainFramebufferCount(); ++i)
    {
        spritePipeline->UpdateSampledImageDescriptor(i, 3, imageViews);
    }
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


	cbSpriteTransform = new GPUBuffer(pDevice);
	cbSpriteTransform->Create(pDevice->GetDeviceProperties().limits.minUniformBufferOffsetAlignment * swapchainFramebufferCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, true);
    pTransformBufferGPUMemory = cbSpriteTransform->Map();

	vertexBuffer = new GPUBuffer(pDevice);
	vertexBuffer->Create(sizeof(VertexPositionTexture) * 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    vertexBuffer->Update(&spriteVertices);

    indexBuffer = new GPUBuffer(pDevice);
    indexBuffer->Create(sizeof(uint16_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    indexBuffer->Update(&spriteIndices);

    nullTexture = new Texture2D(pDevice);
    nullTexture->Create(1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    size_t minUBO_Align = pDevice->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
    size_t dynamic_align = sizeof(glm::mat4);
    if (minUBO_Align > 0)
    {
        dynamic_align = (dynamic_align + minUBO_Align - 1) & ~(minUBO_Align - 1);
    }
    for (int i = 0; i < pDevice->GetSwapchainFramebufferCount(); ++i)
    {
        size_t finalOffset = i * dynamic_align;
        spritePipeline->UpdateUniformBufferDescriptor(i, 1, cbSpriteTransform->GetBuffer(), finalOffset, sizeof(glm::mat4));
    }
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
    spriteTransformBinding.binding = 1;
    spriteTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding cameraTransformBinding{};
    cameraTransformBinding.descriptorCount = 1;
    cameraTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraTransformBinding.binding = 0;
    cameraTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding spriteSampler{};
    spriteSampler.binding = 2;
    spriteSampler.descriptorCount = 1;
    spriteSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    spriteSampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

    VkDescriptorSetLayoutBinding sprite_array{};
    sprite_array.binding = 3;
    sprite_array.descriptorCount = SPRITE_ARRAY_SIZE;
    sprite_array.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sprite_array.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

    VkPushConstantRange spriteIndex{};
    spriteIndex.offset = 0;
    spriteIndex.size = sizeof(int);
    spriteIndex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    spritePipeline->SetPushConstantRange(spriteIndex);
    spritePipeline->RegisterDescriptorSetLayoutBinding(spriteTransformBinding);
    spritePipeline->RegisterDescriptorSetLayoutBinding(spriteSampler);
    spritePipeline->RegisterDescriptorSetLayoutBinding(cameraTransformBinding);
    spritePipeline->RegisterDescriptorSetLayoutBinding(sprite_array);
    

    spritePipeline->Build();
    spritePipeline->CreateDescriptorSets(); //TODO: look into having the pipeline create the descriptor sets
    //on build.
}

void SpriteRenderer::createSpriteSampler()
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

    auto res = vkCreateSampler(pDevice->GetGPU(), &sci, nullptr, &spriteSampler);

    for (int i = 0; i < pDevice->GetSwapchainFramebufferCount(); ++i)
    {
        spritePipeline->UpdateSamplerDescriptor(i, 2, spriteSampler);
    }
}