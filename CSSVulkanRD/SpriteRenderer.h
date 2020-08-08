#pragma once
#include "includes.h"


class GPUBuffer;
class PipelineState;
class Shader;
class GPUMemoryManager;
class DeviceContext;
struct VertexPositionTexture;
class RenderPass;
class GraphicsDevice;
class Texture2D;
struct CommandBuffer;


class SpriteRenderer
{
public:
	SpriteRenderer(GraphicsDevice* pDevice);
	~SpriteRenderer();

	void Initialize(int windowWidth, int windowHeight, int numSwapchainFramebuffers);

	void RenderSprite(Texture2D* Sprite, glm::vec2 position, float rotation);
	void RenderSprite(CommandBuffer* gpuCommandBuffer, Texture2D* Sprite, glm::vec2 position, float rotation);

	void BeginSpriteRenderPass(uint32_t frameIndex);
	void EndSpriteRenderPass(bool submit=false);

	glm::mat4 getTransform() const;

	void RecreatePipelineState();
private:
	GraphicsDevice* pDevice;
	GPUBuffer* cbProjection;
	GPUBuffer* cbSpriteTransform;

	GPUBuffer* vertexBuffer;
	GPUBuffer* indexBuffer;

	PipelineState* spritePipeline;
	Shader* spriteShader;

	uint32_t width;
	uint32_t height;
	uint32_t swapchainFramebufferCount;

	glm::mat4 spriteTransform;
	glm::mat4 cameraTransform;

	void* pcbWorldViewGPUMemory;

	void updateCameraTransform();
	void createBuffers();
	void createPipelineState();

	bool spriteRenderPassActive;
	bool spritePipelineBound;
	uint32_t frameIndex;

	CommandBuffer* spriteCMD;
	void* pTransformBufferGPUMemory;
};