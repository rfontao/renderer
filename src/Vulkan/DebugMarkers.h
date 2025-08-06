#pragma once
#include "pch.h"

#include <string_view>

#include "VulkanDevice.h"

namespace DebugMarkers {

    inline void BufferMarker(const std::shared_ptr<VulkanDevice> &device, VkBuffer buffer, std::string_view name) {
        const VkDebugUtilsObjectNameInfoEXT bufferMarkerInfo{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType = VK_OBJECT_TYPE_BUFFER,
                .objectHandle = (uint64_t) buffer,
                .pObjectName = name.data(),
        };
        vkSetDebugUtilsObjectNameEXT(device->GetDevice(), &bufferMarkerInfo);
    }

    inline void ImageMarker(const std::shared_ptr<VulkanDevice> &device, VkImage image, std::string_view name) {
        const VkDebugUtilsObjectNameInfoEXT imageMarkerInfo{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType = VK_OBJECT_TYPE_IMAGE,
                .objectHandle = (uint64_t) image,
                .pObjectName = name.data(),
        };
        vkSetDebugUtilsObjectNameEXT(device->GetDevice(), &imageMarkerInfo);
    }

    class ScopedMarker {
    public:
        ScopedMarker() = delete;
        ScopedMarker(VkCommandBuffer commandBuffer, std::string_view name) : commandBuffer(commandBuffer) {
            const VkDebugUtilsLabelEXT debugUtil = {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                    .pLabelName = name.data(),
            };
            vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugUtil);
        }

        ScopedMarker(ScopedMarker &&other) = delete;
        ScopedMarker &operator=(const ScopedMarker &other) = delete;

        ScopedMarker(const ScopedMarker &other) = delete;
        ScopedMarker &operator=(ScopedMarker &&other) = delete;

        ~ScopedMarker() { vkCmdEndDebugUtilsLabelEXT(commandBuffer); }

        VkCommandBuffer commandBuffer;
    };
} // namespace DebugMarkers
