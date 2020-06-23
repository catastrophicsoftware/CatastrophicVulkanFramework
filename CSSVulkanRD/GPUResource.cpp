#include "GPUResource.h"
#include "GraphicsDevice.h"

GPUResource::GPUResource(GraphicsDevice* pDevice)
{
	this->pDevice = pDevice;
	gpuMemoryAllocated = false;

	GPU = pDevice->GetGPU();
	physicalDevice = pDevice->GetPhysicalDevice();
}

GPUResource::~GPUResource()
{
}