#pragma once
#include "includes.h"

class GraphicsDevice;

class GPUResource
{
public:
	GPUResource(GraphicsDevice* pDevice);
	GPUResource();
	~GPUResource();

	virtual void Destroy() = 0;
protected:
	GraphicsDevice* pDevice;
};