#include "GPUMemoryManager.h"
#include "includes.h"

GPUMemoryManager::GPUMemoryManager(VkPhysicalDevice device, VkDevice gpu)
{
	physicalDevice = device;
	GPU = gpu;
    allocCount = 0;
    gpuMemoryUsed = 0;
    poolCount = 0;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

GPUMemoryManager::~GPUMemoryManager()
{
    ReleaseAll();
}

GPUMemoryAllocation* GPUMemoryManager::AllocateGPUMemory(VkMemoryRequirements requiredAlloc, VkMemoryPropertyFlags memoryPropertyFlags)
{
    THREAD_LOCK(lock);

    auto gpu_mem = new GPUMemoryAllocation;
    gpu_mem->handleAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    gpu_mem->handleAllocInfo.allocationSize = requiredAlloc.size;
    gpu_mem->handleAllocInfo.memoryTypeIndex = FindCompatibleGPUMemoryType(requiredAlloc.memoryTypeBits, memoryPropertyFlags);
    gpu_mem->allocID = getAllocID();
    gpu_mem->memFlags = memoryPropertyFlags;
    gpu_mem->offset = 0;

    VULKAN_CALL_ERROR(vkAllocateMemory(GPU, &gpu_mem->handleAllocInfo, nullptr, &gpu_mem->handle), "Failed to allocate GPU memory!");

    gpuMemoryUsed += requiredAlloc.size;
    allocationList.push_back(gpu_mem);

    return gpu_mem;
}

GPUMemoryAllocation* GPUMemoryManager::PoolAllocateGPUMemory(VkMemoryRequirements allocReq, VkMemoryPropertyFlags memoryPropertyFlags)
{
    if (memoryPools.size() == 0)
    {
        auto pool = createMemoryPool((1024 * 1024 * 8),memoryPropertyFlags,allocReq);
        //this is a brand new memory pool, we can return offset 0 as the allocation

        auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), 0);
        alloc->handleAllocInfo = pool->handleAllocInfo;

        UsedRange* newUsedRange = new UsedRange;
        newUsedRange->start = 0;
        newUsedRange->size = static_cast<uint32_t>(allocReq.size);

        pool->usedRegions.push_back(newUsedRange);
            
        return alloc;
    }
    else
    {
        for (int i = 0; i < memoryPools.size(); i++)
        {
            auto pool = memoryPools[i];

            if (pool->memFlags != memoryPropertyFlags && memoryPools.size() == 1) //we only have this one pool, and it doesn't match, new pool required
            {
                auto pool = createMemoryPool((1024 * 1024 * 8),memoryPropertyFlags,allocReq);
                //this is a brand new memory pool, we can return offset 0 as the allocation

                auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), 0);
                alloc->handleAllocInfo = pool->handleAllocInfo;

                UsedRange* newUsedRange = new UsedRange;
                newUsedRange->start = 0;
                newUsedRange->size = static_cast<uint32_t>(allocReq.size);

                pool->usedRegions.push_back(newUsedRange);
                    
                return alloc;
            }

            else if(pool->memFlags == memoryPropertyFlags)
            {
                uint32_t usedRegionCount = pool->usedRegions.size();
                uint32_t offset = 0;

                if (usedRegionCount == 0)
                {
                    //no used regions, we can return an alloc to offset 0
                    auto pool = createMemoryPool((1024 * 1024 * 8), memoryPropertyFlags, allocReq);
                    //this is a brand new memory pool, we can return offset 0 as the allocation

                    auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), 0);
                    alloc->handleAllocInfo = pool->handleAllocInfo;

                    UsedRange* newUsedRange = new UsedRange;
                    newUsedRange->start = 0;
                    newUsedRange->size = static_cast<uint32_t>(allocReq.size);

                    pool->usedRegions.push_back(newUsedRange);

                    return alloc;
                }
                else
                {
                    if (usedRegionCount == 1)//only 1 used region, this alloc just has to go after it
                    {
                        auto usedRange = pool->usedRegions[0];
                        offset = usedRange->start;
                        offset += usedRange->size + 1; //advanced fully past alloc

                        if ((offset % allocReq.alignment) != 0)
                        {
                            do
                            {
                                offset += 1;
                            } while ((offset % allocReq.alignment) != 0);

                            //offset is now aligned, we can mark and return the allocation
                            auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), offset);
                            alloc->handleAllocInfo = pool->handleAllocInfo;

                            auto newUsedRegion = new UsedRange();
                            newUsedRegion->start = offset;
                            newUsedRegion->size = static_cast<uint32_t>(allocReq.size);
                            pool->usedRegions.push_back(newUsedRegion);

                            return alloc;
                        }
                    }
                    else
                    {
                        for (int i = 0; i < usedRegionCount; ++i)
                        {
                            UsedRange* currentRegion = pool->usedRegions[i];
                            offset += currentRegion->size + 1; //get past current allocation

                            int nextRangeIndex = i + 1;
                            if (nextRangeIndex > (pool->usedRegions.size() - 1))
                            {
                                //there is no next region, we can align and make the allocation if there is space before the end of the memory
                                do
                                {
                                    offset += 1;
                                } while ((offset % allocReq.alignment) != 0);

                                size_t spaceRemaining = pool->totalSize - offset;

                                if (spaceRemaining >= allocReq.size) //there is space remaining in the pool for this allocation, we can mark and return the allocation
                                {
                                    auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), offset);
                                    alloc->handleAllocInfo = pool->handleAllocInfo;

                                    auto newUsedRegion = new UsedRange();
                                    newUsedRegion->start = offset;
                                    newUsedRegion->size = static_cast<uint32_t>(allocReq.size);
                                    pool->usedRegions.push_back(newUsedRegion);

                                    return alloc;
                                }
                                else
                                {
                                    //no space remaining, we need to create a new pool and can return an offset 0 allocation
                                    
                                    auto pool = createMemoryPool((1024 * 1024 * 8),memoryPropertyFlags,allocReq);
                                    //this is a brand new memory pool, we can return offset 0 as the allocation

                                    auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), 0);
                                    alloc->handleAllocInfo = pool->handleAllocInfo;

                                    UsedRange* newUsedRange = new UsedRange;
                                    newUsedRange->start = 0;
                                    newUsedRange->size = static_cast<uint32_t>(allocReq.size);

                                    pool->usedRegions.push_back(newUsedRange);
                                    
                                    return alloc;
                                }
                            }
                            UsedRange* nextRegion = pool->usedRegions[i + 1];

                            if ((offset % allocReq.alignment) != 0)
                            {
                                do
                                {
                                    offset += 1;
                                } while ((offset % allocReq.alignment) != 0 && offset < nextRegion->start);

                                if ((nextRegion->start - offset) >= allocReq.size) //we have a correctly alligned address, and there is room before hitting the next allocation
                                {
                                    auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), offset);
                                    alloc->handleAllocInfo = pool->handleAllocInfo;

                                    auto newUsedRegion = new UsedRange();
                                    newUsedRegion->start = offset;
                                    newUsedRegion->size = static_cast<uint32_t>(allocReq.size);
                                    pool->usedRegions.push_back(newUsedRegion);

                                    return alloc;
                                }
                                else
                                {
                                    //not enough room.
                                    offset = nextRegion->start; //start at beginning of next region
                                    continue;
                                }
                            }
                            else
                            {
                                //this address is aligned
                                if ((nextRegion->start - offset) >= allocReq.size) //we have a correctly alligned address, and there is room before hitting the next allocation
                                {
                                    auto alloc = new GPUMemoryAllocation(pool->handle, pool->memFlags, getAllocID(), offset);
                                    alloc->handleAllocInfo = pool->handleAllocInfo;

                                    auto newUsedRegion = new UsedRange();
                                    newUsedRegion->start = offset;
                                    newUsedRegion->size = static_cast<uint32_t>(allocReq.size);
                                    pool->usedRegions.push_back(newUsedRegion);

                                    return alloc;
                                }
                                else
                                {
                                    //not enough room.
                                    offset = nextRegion->start; //start at beginning of next region
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void GPUMemoryManager::ReleaseGPUMemory(uint32_t allocID)
{
    THREAD_LOCK(lock);

    for (int i = 0; i < allocationList.size(); ++i)
    {
        if (allocationList[i]->allocID == allocID)
        {
            vkFreeMemory(GPU,allocationList[i]->handle,nullptr);
            allocationList.erase(allocationList.begin() + i);
        }
    }
}

void GPUMemoryManager::ReleaseAll()
{
    for (int i = 0; i < allocationList.size(); ++i)
    {
        ReleaseGPUMemory(allocationList[i]->allocID);
    }
}

uint32_t GPUMemoryManager::getAllocID()
{
    uint32_t id = allocCount;
    allocCount++;
    return id;
}

uint32_t GPUMemoryManager::getPoolID()
{
    uint32_t id = poolCount;
    poolCount++;
    return id;
}

GPUMemoryPool* GPUMemoryManager::createMemoryPool(uint32_t size, VkMemoryPropertyFlags memoryProperties, VkMemoryRequirements allocReq)
{
    GPUMemoryPool* newPool = new GPUMemoryPool;
    newPool->handleAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    newPool->handleAllocInfo.allocationSize = (1024 * 1024 * 8); //8MB chunk of GPU memory
    newPool->handleAllocInfo.memoryTypeIndex = FindCompatibleGPUMemoryType(allocReq.memoryTypeBits,memoryProperties);
    newPool->handleAllocInfo.pNext = nullptr;
    newPool->memFlags = memoryProperties;
    newPool->poolID = getPoolID();

    VULKAN_CALL_ERROR(vkAllocateMemory(GPU, &newPool->handleAllocInfo, nullptr, &newPool->handle), "failed to allocate 8MB GPU memory pool");

    newPool->totalSize = size;
    memoryPools.push_back(newPool);
    return newPool;
}

uint32_t GPUMemoryManager::FindCompatibleGPUMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & memProperties) == memProperties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable GPU memory type!");
}

GPUMemoryAllocation::GPUMemoryAllocation(VkDeviceMemory handle, VkMemoryPropertyFlags memFlags, uint32_t allocID, uint32_t offset)
{
    this->allocID = allocID;
    this->offset = offset;
    this->memFlags = memFlags;
    this->handle = handle;
    this->handleAllocInfo = {};
}

GPUMemoryAllocation::GPUMemoryAllocation()
{
    this->allocID = 0;
    this->handleAllocInfo = {};
    this->handle = 0;
}

GPUMemoryAllocation::~GPUMemoryAllocation()
{

}