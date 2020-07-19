#include "CatastrophicVulkanFramework.h"
#include "GraphicsDevice.h"
#include <windows.h>
#include "GPUBuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include "DeviceContext.h"

void CatastrophicVulkanFrameworkApplication::Run()
{
	InitializeApplicationWindow(800, 600);
	InitializeGraphicsSubsystem();
	InitializeFramebufferResizeHooks();

	MainLoop();

	Shutdown();
}

void CatastrophicVulkanFrameworkApplication::Shutdown()
{
	DestroyResources();
	glfwDestroyWindow(ApplicationWindow);
	glfwTerminate();
}

struct WorldViewProjection
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 projection;
};

void CatastrophicVulkanFrameworkApplication::MainLoop()
{
	Initialize();

	while (!glfwWindowShouldClose(ApplicationWindow))
	{
		glfwPollEvents();

		Update();
		Render();
	}

	DestroyResources();

	pGraphics->WaitForGPUIdle();
}

void CatastrophicVulkanFrameworkApplication::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto graphics = reinterpret_cast<GraphicsDevice*>(glfwGetWindowUserPointer(window));

	ExitProcess(0); //7-19-2020 -- there might be some video driver edge case causing my
	//min/maximization process to get totally fucked. minimization / maximization is not supported for now

	graphics->WaitForGPUIdle();
	graphics->ResizeFramebuffer();
}

void CatastrophicVulkanFrameworkApplication::InitializeApplicationWindow(uint32 width, uint32 height)
{
	WindowWidth = width;
	WindowHeight = height;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	ApplicationWindow = glfwCreateWindow(WindowWidth, WindowHeight, "CSSoftware Vulkan R&D", nullptr, nullptr);
	glfwSetWindowUserPointer(ApplicationWindow, pGraphics);
	glfwSetFramebufferSizeCallback(ApplicationWindow, FramebufferResizeCallback);
}

void CatastrophicVulkanFrameworkApplication::InitializeGraphicsSubsystem()
{
	pGraphics = new GraphicsDevice(ApplicationWindow);
	pGraphics->InitializeVulkan();
}

void CatastrophicVulkanFrameworkApplication::InitializeFramebufferResizeHooks()
{
	glfwSetWindowUserPointer(ApplicationWindow, pGraphics);
}