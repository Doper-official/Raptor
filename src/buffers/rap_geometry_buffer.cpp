#include "buffers/rap_geometry_buffer.hpp"

#include <cassert>
#include <vector>

namespace rap {
    namespace {
        VkFormat findGBufferDepthFormat(RaptorDevice& device) {
            return device.findSupportedFormat(
                {
                    VK_FORMAT_D32_SFLOAT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT
                },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        }
    }

    RaptorGeometryBuffer::RaptorGeometryBuffer(
        RaptorDevice& device,
        VkExtent2D extent)
        : device{ device },
        extent{ extent },
        albedoImage{
            device,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT },
        normalImage{
            device,
            extent,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT },
        depthImage{
            device,
            extent,
            findGBufferDepthFormat(device),
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT } {

        assert(extent.width > 0 &&
            "G-buffer width cannot be zero!");
        assert(extent.height > 0 &&
            "G-buffer height cannot be zero!");
    }
}