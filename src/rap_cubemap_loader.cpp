#include "rap_cubemap_loader.hpp"

//other stuff
#include <stb_image.h>

//std
#include <array>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

namespace rap {

	std::unique_ptr<RaptorCubeMap>
		RaptorCubeMapLoader::loadHorizontalCross(
			RaptorDevice& device,
			const std::filesystem::path& filepath
		) {
		int width = 0;
		int height = 0;
		int channels = 0;

		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> loadedImage{
			stbi_load(
				filepath.string().c_str(),
				&width,
				&height,
				&channels,
				STBI_rgb_alpha
			),
			stbi_image_free
		};

		if (!loadedImage) {
			throw std::runtime_error(
				"failed to load cubemap atlas: " +
				filepath.string()
			);
		}

		if (width <= 0 || height <= 0 ||
			width % 4 != 0 || height % 3 != 0 ||
			width / 4 != height / 3) {
			throw std::runtime_error(
				"Invalid horizontal cross dimensions"
			);
		}

		const uint32_t faceSize =
			static_cast<uint32_t>(width / 4);

		const uint32_t atlasWidth =
			static_cast<uint32_t>(width);

		constexpr std::array<std::array<uint32_t, 2>, 6> positions{ {
			{{2, 1}}, //+X
			{{0, 1}}, //-X
			{{1, 0}}, //+Y
			{{1, 2}}, //-Y
			{{1, 1}}, //+Z
			{{3, 1}} //-Z
		} };

		std::array<std::vector<unsigned char>, 6> faces;

		const size_t faceBytes =
			static_cast<size_t>(faceSize) * faceSize * 4;

		for (size_t face = 0; face < 6; face++) {

			faces[face].resize(faceBytes);

			const uint32_t offsetX =
				positions[face][0] * faceSize;

			const uint32_t offsetY =
				positions[face][1] * faceSize;

			for (uint32_t y = 0; y < faceSize; y++) {

				const size_t sourceOffset =
					(static_cast<size_t>(offsetY + y) * atlasWidth + offsetX) * 4;

				const size_t destinationOffset =
					static_cast<size_t>(y) * faceSize * 4;

				std::copy_n(
					loadedImage.get() + sourceOffset,
					static_cast<size_t>(faceSize) * 4,
					faces[face].data() + destinationOffset
				);
			}
		}

		auto cubemap = std::make_unique<RaptorCubeMap>(
			device,
			faceSize
		);

		cubemap->uploadFaces(faces);

		return cubemap;
	}
} // namespace rap