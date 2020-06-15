#include "CatastrophicVulkanFramework.h"
#include "GraphicsDevice.h"

void CatastrophicVulkanFrameworkApplication::Run()
{
	InitializeApplicationWindow(800, 600);
	InitializeGraphicsSubsystem();

	MainLoop();

	Shutdown();
}

void CatastrophicVulkanFrameworkApplication::Shutdown()
{
	pGraphics->ShutdownVulkan();
	glfwDestroyWindow(ApplicationWindow);
	glfwTerminate();
}

void CatastrophicVulkanFrameworkApplication::MainLoop()
{
	while (!glfwWindowShouldClose(ApplicationWindow))
	{
		glfwWaitEvents();

		pGraphics->BeginRenderPass();
		//all resource / buffer / view / whatever binding for rendered objects using current graphics
		//pipeline state
		pGraphics->EndRenderPass(); //begin and end pass is the process of recording command buffers

		pGraphics->DrawFrame(); //submits, executes command buffers and displays frame
	}

	pGraphics->WaitForGPUIdle();
}

void CatastrophicVulkanFrameworkApplication::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto graphics = reinterpret_cast<GraphicsDevice*>(glfwGetWindowUserPointer(window));
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