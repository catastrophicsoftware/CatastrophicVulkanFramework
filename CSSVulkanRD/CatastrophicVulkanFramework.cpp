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
	std::unique_ptr<GPUBuffer> IndexBuffer = std::make_unique<GPUBuffer>(this->pGraphics);
	std::unique_ptr<GPUBuffer> cbWVP = std::make_unique<GPUBuffer>(this->pGraphics);


	const std::vector<VertexPositionColor> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};



	VertexBuffer->Create(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE);
	VertexBuffer->Update((void*)vertices.data());

	IndexBuffer->Create(sizeof(uint16_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
	IndexBuffer->Update((void*)indices.data());

	cbWVP->Create(sizeof(WorldViewProjection)*3, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE, true);

	WorldViewProjection wvp{};
	wvp.view = glm::lookAt(
		glm::vec3(0, 0, -4), 
		glm::vec3(0, 0, 0),    
		glm::vec3(0, -1, 0)    
	);
	wvp.projection = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 1000.0f);
	wvp.projection[1][1] *= -1;
	wvp.world = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	auto gpu_mem = cbWVP->Map(0, sizeof(WorldViewProjection));

	while (!glfwWindowShouldClose(ApplicationWindow))
	{
		glfwPollEvents();

		int fIndex = pGraphics->PrepareFrame();
		pGraphics->BeginRenderPass();
		auto currentFrame = pGraphics->GetCurrentFrame();
		//all resource / buffer / view / whatever binding for rendered objects using current graphics
		//pipeline state

		VkDeviceSize offsets[] = { 0 };
		VkBuffer binding[] = { VertexBuffer->GetBuffer() };
		vkCmdBindVertexBuffers(currentFrame->cmdBuffer->handle, 0, 1,binding, offsets);
		vkCmdBindIndexBuffer(currentFrame->cmdBuffer->handle, IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(currentFrame->cmdBuffer->handle, 6, 1, 0, 0, 0);

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