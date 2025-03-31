#pragma once

#include "pch.h"
#include "VulkanDevice.h"

#define VK_CHECK(func, msg) if (func != VK_SUCCESS) throw std::runtime_error(msg)

// TODO: Improve
inline std::vector<char> ReadFile(const std::filesystem::path &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file - {}!", filename.string()));
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}