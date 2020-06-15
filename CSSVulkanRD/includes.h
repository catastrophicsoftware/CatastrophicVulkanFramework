#pragma once

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

#include "GraphicsDevice.h"

#define VULKAN_CALL(x) if (x != VK_SUCCESS) {throw std::runtime_error("Vulkan API Call Failed!");}