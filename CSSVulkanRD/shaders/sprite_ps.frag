#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SPRITE_ARRAY_SIZE 32

//resources
layout(binding = 2) uniform sampler   spriteSampler;
layout(binding = 3) uniform texture2D sprites[SPRITE_ARRAY_SIZE];

layout(push_constant) uniform SPRITE_INDEX
{
    int id;
}pc;

//input
layout(location = 0) in vec2 texcoord;


//output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(sprites[pc.id], spriteSampler), texcoord);
}