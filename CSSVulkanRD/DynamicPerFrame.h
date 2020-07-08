#pragma once
#include <vector>
#include <memory>

template <class rType>
class DynamicPerFrame
{
public:
	DynamicPerFrame(uint32_t objectCount);
	~DynamicPerFrame();

private:
	std::vector<std::shared_ptr<rType*>> resources;
	uint32_t count;
};