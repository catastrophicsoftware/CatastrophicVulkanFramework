#pragma once
#include <iostream>
#include <vulkan/vulkan.h>

class CSSVulkanApplication
{
public:
	CSSVulkanApplication();
	~CSSVulkanApplication();

	void Run();

private:
	void InitializeVulkan();
	void MainLoop();
	void Shutdown();

	VkInstance vkInstance;
};