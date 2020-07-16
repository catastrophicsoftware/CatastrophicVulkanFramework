#pragma once
#include "includes.h"

struct ShaderResource
{
	VkShaderModule module;
	VkShaderStageFlagBits stage;
	const char* entrypoint;
	char* bytecode;

	ShaderResource();
	~ShaderResource();
};

static std::vector<char> readFile(const std::string& filename);

class Shader
{
public:
	Shader(VkDevice gpu);
	~Shader();

	void LoadShader(const char* shaderFile, const char* entrypoint, VkShaderStageFlagBits stage);
	void Destroy(VkShaderStageFlags stage);
	void Destroy();
	std::shared_ptr<ShaderResource> GetShader(VkShaderStageFlagBits stage);

	bool StageExists(VkShaderStageFlagBits stage);
private:
	VkDevice GPU;

	std::shared_ptr<ShaderResource> VertexShader;
	std::shared_ptr<ShaderResource> PixelShader;
	std::shared_ptr<ShaderResource> ComputeShader;

	uint32_t vertexStageExists : 1;
	uint32_t fragmentStageExists : 1;
	uint32_t computeStageExists : 1;

	VkShaderModule createShader(const std::vector<char>& bytecode);
};