#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include "CatastrophicVulkanFramework.h"
#include "PipelineState.h"
#include "GPUBuffer.h"
#include "GraphicsDevice.h"
#include <glm/gtc/matrix_transform.hpp>
#include "DeviceContext.h"
#include <Windows.h>
#include "Shader.h"
#include "VertexTypes.h"
#include "Texture2D.h"
#include "EngineThreadPool.h"


struct WorldViewProjection
{
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 projection;
};

class app : public CatastrophicVulkanFrameworkApplication
{
public:
    virtual void Initialize() override;
    virtual void Update() override;
    virtual void Render() override;
    virtual void DestroyResources()override;

    void CreatePipelineState(); //todo: bring this into the CatastrophicVulkanFrameworkApplication class
private:
    PipelineState* Pipeline;
    Shader* simpleShader;

    ThreadPool* threadPool;
};


int main()
{
    app* pApp = new app();

    try {
        pApp->Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void app::Initialize()
{
    std::function<void()> callback = std::bind(&app::CreatePipelineState, this);
    pGraphics->SetPipelineStateRecreateCallback(callback);

    simpleShader = new Shader(pGraphics->GetGPU());
    simpleShader->LoadShader("shaders\\vs.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
    simpleShader->LoadShader("shaders\\ps.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);

    CreatePipelineState();
}

void app::Update()
{
}

#define WAIT_GPU_FENCE(fence){if(vkGetFenceStatus(pGraphics->GetGPU(),fence) != VK_SUCCESS){vkWaitForFences(pGraphics->GetGPU(),1,&fence,VK_TRUE,INFINITE);}}

void app::Render()
{
    int fIndex = pGraphics->PrepareFrame();

    auto currentFrame = pGraphics->GetCurrentFrame();
    VkCommandBuffer cmd = currentFrame->cmdBuffer->handle;
    currentFrame->cmdBuffer->Begin();
  
    //int fIndex = pGraphics->PrepareFrame();
    pGraphics->BeginRenderPass();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->Pipeline->GetPipeline());


    pGraphics->EndRenderPass(); //begin and end pass is the process of recording "main" command buffer

    pGraphics->DrawFrame(); //submits, executes command buffers and displays frame

    Sleep(13);
}

void app::DestroyResources()
{

}

void app::CreatePipelineState()
{
    Pipeline = new PipelineState(pGraphics->GetGPU(), pGraphics->GetSwapchainFramebufferCount());

    auto swapExt = pGraphics->GetSwapchainExtent();
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapExt.width;
    viewport.height = (float)swapExt.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    Pipeline->SetViewport(viewport);

    VertexInputData vsInputData;
    vsInputData.vertexBindingDesc = VertexPositionTexture::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> vertexAttrDesc = { VertexPositionTexture::GetVertexAttributeDescriptions()[0],VertexPositionTexture::GetVertexAttributeDescriptions()[1] }; //HACK!
    vsInputData.vertexInputAttributeDescriptions = vertexAttrDesc;

    Pipeline->SetVertexInput(vsInputData);
    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = pGraphics->GetSwapchainExtent();

    Pipeline->SetScissor(scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    Pipeline->SetRasterizerState(rasterizer);

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    Pipeline->SetMultisamplingState(multisampling);

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    BlendState blendState;
    blendState.blendState = colorBlending;
    blendState.colorBlendAttachment = colorBlendAttachment;

    Pipeline->SetBlendState(blendState);
    Pipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    Pipeline->SetPrimitiveRestartEnable(VK_FALSE);
    Pipeline->SetRenderPass(pGraphics->GetRenderPass());
    Pipeline->SetDescriptorPool(ImmediateContext->GetDescriptorPool());
    Pipeline->SetShader(simpleShader); 


    //TODO: create descriptor set layout bindings here


    Pipeline->Build();
    Pipeline->CreateDescriptorSets();

    pGraphics->SetPipelineState(Pipeline); //maybe this shouldn't be here
}