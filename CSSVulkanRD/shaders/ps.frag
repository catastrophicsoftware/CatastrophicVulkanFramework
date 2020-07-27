#version 450
#extension GL_ARB_separate_shader_objects : enable

//resources
layout(binding = 1) uniform sampler2D textureSampler;


//input
layout(location = 0) in vec2 texcoord;


//output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(textureSampler,texcoord);
}