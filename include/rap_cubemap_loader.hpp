#pragma once

//my libs
#include "rap_device.hpp"
#include "rap_cubemap.hpp"

//std
#include <filesystem>
#include <memory>

namespace rap {

	class RaptorCubeMapLoader {
	public:

		static std::unique_ptr<RaptorCubeMap> loadHorizontalCross(
			RaptorDevice& device,
			const std::filesystem::path& filepath
		);
	};
}