#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <optional>
#include <array>
#include <string>
#include <memory>
#include <limits>
#include <set>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <unordered_map>
