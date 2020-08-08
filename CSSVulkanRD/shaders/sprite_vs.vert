#version 450
#extension GL_ARB_separate_shader_objects : enable

//input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexcoord;

//output
layout(location = 0) out vec2 outTexcoord;

//resources
layout(binding = 0) uniform SpriteProjection
{
	mat4 matrix;
}proj;

layout(binding = 1) uniform PerSpriteTransform
{
	mat4 transform;
}perSprite;

void main()
{
	outTexcoord = inTexcoord;

	gl_Position = proj.matrix * perSprite.transform * vec4(inPosition.xy, 0.0, 1.0);
}