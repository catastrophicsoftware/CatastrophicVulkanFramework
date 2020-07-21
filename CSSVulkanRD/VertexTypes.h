#pragma once
#include "includes.h"

struct VertexPositionColor
{
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> GetVertexAttributeDescriptions();
};

struct VertexPositionTexture
{
    glm::vec2 position;
    glm::vec2 texcoord;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
};