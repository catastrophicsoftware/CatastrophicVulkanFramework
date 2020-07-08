#include "Shader.h"

Shader::Shader(VkDevice gpu)
{
	GPU = gpu;
}

Shader::~Shader()
{
}

void Shader::LoadShader(const char* shaderFile, const char* entrypoint, VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
		{
			auto code = readFile(shaderFile);
			VkShaderModule module = createShader(code);

			VertexShader = std::make_shared<ShaderResource>();
			VertexShader->entrypoint = entrypoint;
			VertexShader->module = module;
			VertexShader->stage = stage;
			VertexShader->bytecode = code.data(); //unsure now necessary it will be to keep cpu copy of shader bytecode indefinitely

			break;
		}
		case VK_SHADER_STAGE_FRAGMENT_BIT:
		{
			auto code = readFile(shaderFile);
			VkShaderModule module = createShader(code);

			PixelShader = std::make_shared<ShaderResource>();
			PixelShader->entrypoint = entrypoint;
			PixelShader->module = module;
			PixelShader->stage = stage;
			PixelShader->bytecode = code.data(); //unsure now necessary it will be to keep cpu copy of shader bytecode indefinitely

			break;
		}
		case VK_SHADER_STAGE_COMPUTE_BIT:
		{
			auto code = readFile(shaderFile);
			VkShaderModule module = createShader(code);

			ComputeShader = std::make_shared<ShaderResource>();
			ComputeShader->entrypoint = entrypoint;
			ComputeShader->module = module;
			ComputeShader->stage = stage;
			ComputeShader->bytecode = code.data(); //unsure now necessary it will be to keep cpu copy of shader bytecode indefinitely

			break;
		}
	}
}

void Shader::Destroy(VkShaderStageFlags stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
		{
			if (VertexShader)
			{
				vkDestroyShaderModule(GPU, VertexShader->module,nullptr);
				VertexShader = nullptr;
			}
			break;
		}
		case VK_SHADER_STAGE_FRAGMENT_BIT:
		{
			if (PixelShader)
			{
				vkDestroyShaderModule(GPU, PixelShader->module, nullptr);
				PixelShader = nullptr;
			}
			break;
		}
		case VK_SHADER_STAGE_COMPUTE_BIT:
		{
			if (ComputeShader)
			{
				vkDestroyShaderModule(GPU, ComputeShader->module, nullptr);
				ComputeShader = nullptr;
			}
		}
	}
}

void Shader::Destroy()
{
	if (VertexShader)
	{
		vkDestroyShaderModule(GPU, VertexShader->module, nullptr);
		VertexShader = nullptr;
	}
	if (PixelShader)
	{
		vkDestroyShaderModule(GPU, PixelShader->module, nullptr);
		PixelShader = nullptr;
	}
	if (ComputeShader)
	{
		vkDestroyShaderModule(GPU, ComputeShader->module, nullptr);
		ComputeShader = nullptr;
	}
}

std::shared_ptr<ShaderResource> Shader::GetShader(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
		{
			return VertexShader;
			break;
		}
		case VK_SHADER_STAGE_FRAGMENT_BIT:
		{
			return PixelShader;
			break;
		}
		case VK_SHADER_STAGE_COMPUTE_BIT:
		{
			return ComputeShader;
			break;
		}
	}
}

VkShaderModule Shader::createShader(const std::vector<char>& bytecode)
{
	VkShaderModuleCreateInfo createInfo{};
	VkShaderModule shaderModule;

	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
	createInfo.codeSize = bytecode.size();

	if (vkCreateShaderModule(GPU, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader");
	}
	return shaderModule;
}

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

ShaderResource::ShaderResource()
{
}

ShaderResource::~ShaderResource()
{
}
