#include "VertexTypes.h"

std::array<VkVertexInputAttributeDescription, 2> VertexPositionColor::GetVertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attrDescriptions;
    attrDescriptions[0].binding = 0;
    attrDescriptions[0].location = 0;
    attrDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescriptions[0].offset = offsetof(VertexPositionColor, position);

    attrDescriptions[1].binding = 0;
    attrDescriptions[1].location = 1;
    attrDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescriptions[1].offset = offsetof(VertexPositionColor, color);

    return attrDescriptions;
}

VkVertexInputBindingDescription VertexPositionColor::GetBindingDescription()
{
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(VertexPositionColor);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindDesc;
}

VertexPositionTexture::VertexPositionTexture(glm::vec2 position, glm::vec2 uv)
{
    this->position = position;
    this->texcoord = uv;
}

VkVertexInputBindingDescription VertexPositionTexture::GetBindingDescription()
{
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(VertexPositionTexture);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindDesc;
}

std::array<VkVertexInputAttributeDescription, 2> VertexPositionTexture::GetVertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attrDesc;

    attrDesc[0].binding = 0;
    attrDesc[0].location = 0;
    attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDesc[0].offset = offsetof(VertexPositionTexture, position);

    attrDesc[1].binding = 0;
    attrDesc[1].location = 1;
    attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrDesc[1].offset = offsetof(VertexPositionTexture, texcoord);

    return attrDesc;
}