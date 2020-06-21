#include "CatastrophicVulkanFramework.h"
#include "GraphicsDevice.h"
#include <windows.h>
#include "GPUBuffer.h"

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
	pGraphics->ShutdownVulkan();
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
	//temp
	std::unique_ptr<GPUBuffer> VertexBuffer = std::make_unique<GPUBuffer>(this->pGraphics);
	std::unique_ptr<GPUBuffer> cbWVP = std::make_unique<GPUBuffer>(this->pGraphics);


	const std::vector<VertexPositionColor> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	VertexBuffer->Create(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE);

	VertexBuffer->FillBuffer((void*)vertices.data());

	while (!glfwWindowShouldClose(ApplicationWindow))
	{
		glfwPollEvents();

		pGraphics->PrepareFrame();
		pGraphics->BeginRenderPass();
		VkCommandBuffer activeCMDBuffer = pGraphics->GetActiveCommandBuffer();
		//all resource / buffer / view / whatever binding for rendered objects using current graphics
		//pipeline state

		VkDeviceSize offsets[] = { 0 };
		VkBuffer binding[] = { VertexBuffer->GetBuffer() };
		vkCmdBindVertexBuffers(activeCMDBuffer, 0, 1,binding, offsets);
		vkCmdDraw(activeCMDBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		pGraphics->EndRenderPass(); //begin and end pass is the process of recording command buffer

		pGraphics->DrawFrame(); //submits, executes command buffers and displays frame

		Sleep(13);
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

void CatastrophicVulkanFrameworkApplication::InitializeFramebufferResizeHooks()
{
	glfwSetWindowUserPointer(ApplicationWindow, pGraphics);
}