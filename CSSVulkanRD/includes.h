#pragma once

#define CATASTROPHIC_PLATFORM_WINDOWS

#include <Windows.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <array>
#include <thread>
#include <mutex>

using namespace std;

typedef uint32_t uint32;

#define VULKAN_CALL(x) if (x != VK_SUCCESS) {throw std::runtime_error("Vulkan API Call Failed!"); }
#define VULKAN_CALL_ERROR(x,error_msg) if (x != VK_SUCCESS) {throw std::runtime_error(error_msg); }
#define THREAD_LOCK(mutexObj) {std::lock_guard<std::mutex> _threadLockObj(mutexObj);}
#define WAIT_GPU_FENCE(fence){if(vkGetFenceStatus(pGraphics->GetGPU(),fence) != VK_SUCCESS){vkWaitForFences(pGraphics->GetGPU(),1,&fence,VK_TRUE,INFINITE);}}
