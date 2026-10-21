#pragma once
#include "rap_device.hpp"

//my libs
#include "rap_image.hpp"

namespace rap {

	class RaptorGeometryBuffer {
	public:
		RaptorGeometryBuffer(
			RaptorDevice& device,
			VkExtent2D extent
		);

		~RaptorGeometryBuffer() = default;

		RaptorGeometryBuffer(const RaptorGeometryBuffer&) = delete;
		RaptorGeometryBuffer& operator=(const RaptorGeometryBuffer&) = delete;

		VkImageView getAlbedoImageView() const {
			return albedoImage.getImageView();
		}

		VkImageView getNormalImageView() const {
			return normalImage.getImageView();
		}

		VkImageView getDepthImageView() const {
			return depthImage.getImageView();
		}

		VkExtent2D getExtent() const {
			return extent;
		}

		VkFormat getDepthFormat() const {
			return depthImage.getFormat();
		}

	private:
		RaptorDevice& device;
		VkExtent2D extent{};

		RaptorImage albedoImage;
		RaptorImage normalImage;
		RaptorImage depthImage;
	};

} // namespace rap