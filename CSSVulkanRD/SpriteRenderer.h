#pragma once
#include "includes.h"

#define SPRITE_ARRAY_SIZE 32

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

struct SpriteTexture
{
	Texture2D* pTexture;
	int spriteID;
};


class SpriteRenderer
{
public:
	SpriteRenderer(GraphicsDevice* pDevice);
	~SpriteRenderer();

	void Initialize(int windowWidth, int windowHeight, int numSwapchainFramebuffers);

	void RenderSprite(Texture2D* Sprite, glm::vec2 position, float rotation);

	void BeginSpriteRenderPass(CommandBuffer* pGPUCommandBuffer, uint32_t frameIndex);
	void EndSpriteRenderPass(bool submit=false);

	glm::mat4 getTransform() const;

	void RecreatePipelineState();

	void SetSprites(std::vector<Texture2D*> sprite_set);
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

	void* pTransformBufferGPUMemory;

	std::vector<SpriteTexture>  spriteTextures;
	std::vector<VkFence> spriteFences;
	CommandBuffer* pActiveSpriteCMD;

	Texture2D* nullTexture;

	VkSampler spriteSampler;
	void createSpriteSampler();
};