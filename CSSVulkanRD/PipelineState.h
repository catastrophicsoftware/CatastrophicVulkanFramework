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
	PipelineState(VkDevice gpu, uint32_t numFramebuffers);
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
	void SetRenderPass(VkRenderPass pass);

	VkDescriptorSet GetDescriptorSet(uint32_t index);
	void CreateDescriptorSets();
	void RegisterDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding binding);
	void SetDescriptorPool(VkDescriptorPool pool); //watch out for this
	void UpdateUniformBufferDescriptor(uint32_t descriptorSetIndex, uint32_t descriptorBindingIndex, VkBuffer gpuBuffer, VkDeviceSize bindOffset, VkDeviceSize bindSize);
	void UpdateStorageBufferDescriptor(uint32_t descriptorSetIndex, uint32_t descriptorBindingIndex, VkBuffer gpuBuffer, VkDeviceSize bindOffset, VkDeviceSize bindSize);

	void Build(bool isComputePipeline=false);
	VkPipeline GetPipeline() const;
	VkPipelineLayout GetPipelineLayout() const; //probably replace this with something better

	void Destroy();
private:
	VkPipeline pipeline;
	Shader* pShader;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkPushConstantRange> pushConstantRanges;
	std::vector<VkDescriptorSet> descriptorSets; //one per swapchain image. TODO: methods to get, and update
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

	void createDescriptorSetLayout();
	void createDescriptorSets();
	void createPipelineLayout();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

	VertexInputData vertexInputData;

	VkViewport viewport;
	VkPipelineViewportStateCreateInfo viewportState;
	VkRect2D scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerState;

	VkPipelineMultisampleStateCreateInfo multisampleState;

	BlendState blendState;

	VkPipelineLayoutCreateInfo  pipelineLayoutInfo;
	VkPipelineLayout pipelineLayout;
	VkGraphicsPipelineCreateInfo pipelineInfo;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	VkRenderPass renderPass;

	VkDescriptorPool descriptorPool;

	VkDevice GPU;

	bool dirty;
	uint32_t numFramebuffers;

	//--compute specific pipeline state
	VkComputePipelineCreateInfo computePipelineInfo;
};