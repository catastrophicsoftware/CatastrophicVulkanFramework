#pragma once
#include "includes.h"

class Shader;

struct VertexInputData
{
	VertexInputData();
	VertexInputData(VkVertexInputBindingDescription bindDesc, std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions);
	~VertexInputData();

	VkVertexInputBindingDescription vertexBindingDesc;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
};

struct BlendState
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo blendState;
};

class PipelineState
{
public:
	PipelineState(VkDevice gpu);
	~PipelineState();

	void SetShader(Shader* pShader);
	void SetViewport(VkViewport viewport);
	void SetVertexInput(VertexInputData info);
	void SetScissor(VkRect2D scissor);
	void SetRasterizerState(VkPipelineRasterizationStateCreateInfo rasterizerState);
	void SetMultisamplingState(VkPipelineMultisampleStateCreateInfo multisampleState);
	void SetBlendState(BlendState blendState);
	void SetPushConstantRange(VkPushConstantRange range);
	void SetPrimitiveTopology(VkPrimitiveTopology topology);
	void SetPrimitiveRestartEnable(VkBool32 primitiveRestartEnabled);
	void SetColorBlendState(VkPipelineColorBlendStateCreateInfo colorBlendState);
	void SetRenderPass(VkRenderPass pass);


	VkDescriptorSet GetDescriptorSet(uint32_t index);
	void RegisterDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding binding);
	void SetDescriptorPool(VkDescriptorPool pool); //watch out for this


	void Build();
private:
	VkPipeline pipeline;
	Shader* pShader;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkPushConstantRange> pushConstantRanges;
	std::vector<VkDescriptorSet> descriptorSets; //one per swapchain image. TODO: methods to get, and update
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

	void createDescriptorSetLayout();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

	VertexInputData vertexInputData;

	VkViewport viewport;
	VkPipelineViewportStateCreateInfo viewportState;
	VkRect2D scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerState;

	VkPipelineMultisampleStateCreateInfo multisampleState;

	BlendState blendState;

	VkPipelineColorBlendStateCreateInfo colorBlendState;

	VkPipelineLayoutCreateInfo  pipelineLayoutInfo;
	VkPipelineLayout pipelineLayout;
	VkGraphicsPipelineCreateInfo pipelineInfo;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	VkRenderPass renderPass;

	VkDescriptorPool descriptorPool;

	VkDevice GPU;

	bool dirty;
};