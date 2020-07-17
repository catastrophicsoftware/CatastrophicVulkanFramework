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

const std::vector<VertexPositionColor> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
    };

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


private:
    PipelineState* Pipeline;
    std::unique_ptr<GPUBuffer> VertexBuffer;
    std::unique_ptr<GPUBuffer> IndexBuffer;
    std::unique_ptr<GPUBuffer> cbWVP;
    Shader* shader;
    WorldViewProjection wvp;
};



int main() {
    //GraphicsDevice* pApp = new GraphicsDevice();

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
    shader = new Shader(pGraphics->GetGPU());
    shader->LoadShader("shaders\\vs.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
    shader->LoadShader("shaders\\ps.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);

    VertexBuffer = std::make_unique<GPUBuffer>(pGraphics);
    IndexBuffer = std::make_unique<GPUBuffer>(pGraphics);
    cbWVP = std::make_unique<GPUBuffer>(pGraphics);

    VertexBuffer->Create(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    	VK_SHARING_MODE_EXCLUSIVE);
    VertexBuffer->Update((void*)vertices.data());

    IndexBuffer->Create(sizeof(uint16_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    IndexBuffer->Update((void*)indices.data());

    cbWVP->Create(sizeof(WorldViewProjection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    	VK_SHARING_MODE_EXCLUSIVE, true);

    wvp.view = glm::lookAt(
    	glm::vec3(0, 0, -4), 
    	glm::vec3(0, 0, 0),    
    	glm::vec3(0, -1, 0)    
    );

    wvp.projection = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 1000.0f);
    wvp.projection[1][1] *= -1;
    wvp.world = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    auto gpu_mem = cbWVP->Map(0, sizeof(WorldViewProjection));
    memcpy(gpu_mem, &wvp, sizeof(WorldViewProjection));
    cbWVP->UnMap();

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
    vsInputData.vertexBindingDesc = VertexPositionColor::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> vertexAttrDesc = { VertexPositionColor::GetVertexAttributeDescriptions()[0],VertexPositionColor::GetVertexAttributeDescriptions()[1] }; //HACK!
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
    Pipeline->SetDescriptorPool(pGraphics->GetDescriptorPool());
    Pipeline->SetShader(shader);

    VkDescriptorSetLayoutBinding wvpBinding{};

    wvpBinding.binding = 0;
    wvpBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    wvpBinding.descriptorCount = 1;
    wvpBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    Pipeline->RegisterDescriptorSetLayoutBinding(wvpBinding);
    Pipeline->Build();

    pGraphics->SetPipelineState(Pipeline);
}

void app::Update()
{
}

void app::Render()
{
    int fIndex = pGraphics->PrepareFrame();
    pGraphics->BeginRenderPass();
    auto currentFrame = pGraphics->GetCurrentFrame();
    VkCommandBuffer cmd = currentFrame->cmdBuffer->handle;
    //all resource / buffer / view / whatever binding for rendered objects using current graphics
    //pipeline state

    VkDeviceSize offsets[] = { 0 };
    VkBuffer binding[] = { VertexBuffer->GetBuffer() };
    vkCmdBindVertexBuffers(cmd, 0, 1,binding, offsets);
    vkCmdBindIndexBuffer(cmd, IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

    pGraphics->GetPipelineState()->UpdateUniformBufferDescriptor(fIndex, 0, cbWVP->GetBuffer(), 0, sizeof(WorldViewProjection));

    VkDescriptorSet currentDescriptor = pGraphics->GetPipelineState()->GetDescriptorSet(fIndex);
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pGraphics->GetPipelineState()->GetPipelineLayout(), 0, 1, &currentDescriptor, 0, nullptr);

    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

    pGraphics->EndRenderPass(); //begin and end pass is the process of recording command buffer

    pGraphics->DrawFrame(); //submits, executes command buffers and displays frame

    Sleep(13);
}

void app::DestroyResources()
{
}