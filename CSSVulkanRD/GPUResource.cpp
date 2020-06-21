#include "GPUResource.h"
#include "GraphicsDevice.h"

GPUResource::GPUResource(GraphicsDevice* pDevice)
{
	this->pDevice = pDevice;
}

GPUResource::~GPUResource()
{
}