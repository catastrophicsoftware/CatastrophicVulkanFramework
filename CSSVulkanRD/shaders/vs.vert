#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inputTexcoord;

layout(location = 0) out vec2 texCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 world;
    mat4 view;
    mat4 projection;
}perFrame;


void main()
{
    gl_Position = perFrame.projection * perFrame.view * perFrame.world * vec4(inPosition,0.0,1.0);

    texCoord = inputTexcoord;
}