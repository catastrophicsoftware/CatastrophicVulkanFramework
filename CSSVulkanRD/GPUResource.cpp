#include "GPUResource.h"
#include "GraphicsDevice.h"
#include "GPUMemoryManager.h"

GPUResource::GPUResource(GraphicsDevice* pDevice)
{
	this->pDevice = pDevice;
	gpuMemoryAllocated = false;

	GPU = pDevice->GetGPU();
	physicalDevice = pDevice->GetPhysicalDevice();
	pAllocator = pDevice->GetMainGPUMemoryAllocator();
}

GPUResource::~GPUResource()
{
}

void GPUResource::Destroy()
{
}

void* GPUResource::Map()
{
	if (mappable && !mapped)
	{
		void* pGPUMemoryRegion = pAllocator->VMA_MapMemory(gpuMemory);
		mapped = true;
		return pGPUMemoryRegion;
	}
	throw std::exception("this should not happen");
}

void GPUResource::UnMap()
{
	if (mappable && mapped)
	{
		pAllocator->VMA_UnmapMemory(gpuMemory);
		mapped = false;
	}
}