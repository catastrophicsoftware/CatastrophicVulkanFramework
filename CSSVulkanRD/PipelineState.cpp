#include "PipelineState.h"
#include "Shader.h"

PipelineState::PipelineState(VkDevice gpu, uint32_t numFramebuffers)
{
	GPU = gpu;

	inputAssembly = {};
	vertexInputInfo = {};
	pipelineInfo = {};
	pipelineLayoutInfo = {};
	descriptorSetLayoutCreateInfo = {};
	computePipelineInfo = {};

	this->numFramebuffers = numFramebuffers;
	descriptorSets.resize(numFramebuffers); //look into these in the context of compute shaders. depending on the compute pipeline it may not be necessary to have one
	//descriptor set per swapchain image

	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

	dirty = false;
}

PipelineState::~PipelineState()
{
}

void PipelineState::SetShader(Shader* pShader)
{
	this->pShader = pShader;

	if (pShader->StageExists(VK_SHADER_STAGE_VERTEX_BIT))
	{
		auto vs = pShader->GetShader(VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo vertexStageInfo = {};
		vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStageInfo.module = vs->module;
		vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStageInfo.pName = vs->entrypoint;

		shaderStages.push_back(vertexStageInfo);
	}
	if (pShader->StageExists(VK_SHADER_STAGE_FRAGMENT_BIT))
	{
		auto ps = pShader->GetShader(VK_SHADER_STAGE_FRAGMENT_BIT);
		VkPipelineShaderStageCreateInfo fragmentStageInfo = {};
		fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStageInfo.module = ps->module;
		fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStageInfo.pName = ps->entrypoint;

		shaderStages.push_back(fragmentStageInfo);
	}
	if (pShader->StageExists(VK_SHADER_STAGE_COMPUTE_BIT))
	{
		auto cs = pShader->GetShader(VK_SHADER_STAGE_COMPUTE_BIT);
		VkPipelineShaderStageCreateInfo computeStageInfo = {};
		computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeStageInfo.module = cs->module;
		computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeStageInfo.pName = cs->entrypoint;

		shaderStages.push_back(computeStageInfo);
	}

	dirty = true;
}

void PipelineState::SetViewport(VkViewport viewport)
{
	this->viewport = viewport;
	dirty = true;
}

void PipelineState::SetVertexInput(VertexInputData data)
{
	this->vertexInputData = data;
	dirty = true;
}

void PipelineState::SetScissor(VkRect2D scissor)
{
	this->scissor = scissor;
	dirty = true;
}

void PipelineState::SetRasterizerState(VkPipelineRasterizationStateCreateInfo rasterizerState)
{
	this->rasterizerState = rasterizerState;
	dirty = true;
}

void PipelineState::SetMultisamplingState(VkPipelineMultisampleStateCreateInfo multisampleState)
{
	this->multisampleState = multisampleState;
	dirty = true;
}

void PipelineState::SetBlendState(BlendState blendState)
{
	this->blendState = blendState;
	dirty = true;
}

void PipelineState::SetPushConstantRange(VkPushConstantRange range)
{
	throw std::runtime_error("not impl");
	dirty = true;
}

void PipelineState::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
	inputAssembly.topology = topology;
	dirty = true;
}

void PipelineState::SetPrimitiveRestartEnable(VkBool32 primitiveRestartEnabled)
{
	inputAssembly.primitiveRestartEnable = primitiveRestartEnabled;
	dirty = true;
}

void PipelineState::SetRenderPass(VkRenderPass pass)
{
	renderPass = pass;
	dirty = true;
}

VkDescriptorSet PipelineState::GetDescriptorSet(uint32_t index)
{
	assert(index <= descriptorSets.size());

	return descriptorSets[index];
}

void PipelineState::CreateDescriptorSets()
{
	createDescriptorSets();
}

void PipelineState::RegisterDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding binding)
{
	descriptorSetLayoutBindings.push_back(binding);
	dirty = true;
}

void PipelineState::SetDescriptorPool(VkDescriptorPool pool)
{
	descriptorPool = pool;
}

//todo: 7-26-2020 look into batching these descriptor writes per frame
void PipelineState::UpdateUniformBufferDescriptor(uint32_t descriptorSetIndex, uint32_t descriptorBindingIndex, VkBuffer gpuBuffer, VkDeviceSize bindOffset, VkDeviceSize bindSize)
{
	assert(descriptorSetIndex <= descriptorSets.size());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = gpuBuffer;
	bufferInfo.offset = bindOffset;
	bufferInfo.range = bindSize;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets[descriptorSetIndex];
	write.dstBinding = descriptorBindingIndex;
	write.dstArrayElement = 0; //what the fuck is this
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(GPU, 1, &write, 0, nullptr);
}

//todo: 7-26-2020 look into batching these descriptor writes per frame
void PipelineState::UpdateStorageBufferDescriptor(uint32_t descriptorSetIndex, uint32_t descriptorBindingIndex, VkBuffer gpuBuffer, VkDeviceSize bindOffset, VkDeviceSize bindSize)
{
	assert(descriptorSetIndex <= descriptorSets.size());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = gpuBuffer;
	bufferInfo.offset = bindOffset;
	bufferInfo.range = bindSize;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets[descriptorSetIndex];
	write.dstBinding = descriptorBindingIndex;
	write.dstArrayElement = 0; //what the fuck is this
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(GPU, 1, &write, 0, nullptr);
}

//todo: 7-26-2020 look into batching these descriptor writes per frame
void PipelineState::UpdateCombinedImageDescriptor(uint32_t descriptorSetIndex, uint32_t descriptorBindingIndex, VkImageView imageView, VkSampler imageSampler)
{
	assert(descriptorSetIndex <= descriptorSets.size());

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSets[descriptorSetIndex];
	write.dstBinding = descriptorBindingIndex;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = imageSampler;
	write.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(GPU, 1, &write, 0, nullptr);
}

void PipelineState::Build(bool isComputePipeline)
{
	if (dirty && !isComputePipeline)
	{
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputData.vertexInputAttributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &vertexInputData.vertexBindingDesc;
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputData.vertexInputAttributeDescriptions.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;

		viewportState.viewportCount = 1; // 1 viewport supported at this time
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1; // 1 scissor supported at this time
		viewportState.pScissors = &scissor;

		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pColorBlendState = &blendState.blendState;

		createDescriptorSetLayout(); //at this point build the descriptor set layout from descriptor set bindings

		createPipelineLayout();

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; //subpasses not yet supported
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VULKAN_CALL_ERROR(vkCreateGraphicsPipelines(GPU, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "failed to create graphics pipeline");
		dirty = false;
	}
	if (dirty && isComputePipeline)
	{
		assert(shaderStages.size() == 1);

		createDescriptorSetLayout();
		createPipelineLayout();

		computePipelineInfo.stage = shaderStages[0];
		computePipelineInfo.layout = pipelineLayout;

		VULKAN_CALL_ERROR(vkCreateComputePipelines(GPU, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &pipeline), "failed to create compute pipeline!");
		dirty = false;
	}
}

VkPipeline PipelineState::GetPipeline() const
{
	return pipeline;
}

VkPipelineLayout PipelineState::GetPipelineLayout() const
{
	return pipelineLayout;
}

void PipelineState::Destroy()
{
	vkDestroyPipeline(GPU, pipeline, nullptr);
	vkDestroyPipelineLayout(GPU, pipelineLayout, nullptr);

	for (int i = 0; i < descriptorSets.size(); ++i)
	{
		descriptorSets[i] = nullptr;
	}

	descriptorSets.resize(numFramebuffers);

	dirty = true;
}

void PipelineState::createDescriptorSetLayout()
{
	descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();
	VULKAN_CALL_ERROR(vkCreateDescriptorSetLayout(GPU, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout), "failed to create descriptor set layout");
}

void PipelineState::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(numFramebuffers, descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = numFramebuffers;
	allocInfo.pSetLayouts = layouts.data();

	VULKAN_CALL_ERROR(vkAllocateDescriptorSets(GPU, &allocInfo, descriptorSets.data()), "failed to allocate descriptor sets");
}

void PipelineState::createPipelineLayout()
{
	if (pushConstantRanges.size() > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
	}
	else
	{
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
	}

	pipelineLayoutInfo.setLayoutCount = 1; //currently 1 descriptor set supported
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	VULKAN_CALL_ERROR(vkCreatePipelineLayout(GPU, &pipelineLayoutInfo, nullptr, &pipelineLayout), "failed to create graphics pipeline layout");
}

VertexInputData::VertexInputData()
{
}

VertexInputData::VertexInputData(VkVertexInputBindingDescription bindDesc, std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	this->vertexBindingDesc = bindDesc;
	this->vertexInputAttributeDescriptions = attributeDescriptions;
}

VertexInputData::~VertexInputData()
{
}