#pragma once
#include "includes.h"

class GraphicsDevice;

class CatastrophicVulkanFrameworkApplication
{
public:
	void Run();
protected:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void DestroyResources() = 0;
private:
	GraphicsDevice* pGraphics;
	GLFWwindow* ApplicationWindow;
	uint32 WindowWidth;
	uint32 WindowHeight;
	bool Fullscreen;

	void MainLoop();
	void Shutdown();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	void InitializeApplicationWindow(uint32 width, uint32 height);
	void InitializeGraphicsSubsystem();
	void InitializeFramebufferResizeHooks();
};