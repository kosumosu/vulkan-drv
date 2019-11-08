#pragma once

#include "SelfDestroyable.h"

#include "utils.hpp"

#include <vulkan/vulkan.hpp>

#include <boost/noncopyable.hpp>

#include <filesystem>
#include <fstream>

class Pipeline // : private boost::noncopyable
{
	vk::Device& device_;

public:
	explicit Pipeline(vk::Device& device)
		: device_(device)
	{
		const auto vertexShaderModule = makeSelfDestroyable(
			LoadShaderModule((std::filesystem::path(DRIVER_DATA_DIRECTORY_NAME) / "shader.vert.spv").wstring().c_str()),
			[&](vk::ShaderModule& shader) { device_.destroyShaderModule(shader); });
		const auto fragmentShaderModule = makeSelfDestroyable(
			LoadShaderModule((std::filesystem::path(DRIVER_DATA_DIRECTORY_NAME) / "shader.frag.spv").wstring().c_str()),
			[&](vk::ShaderModule& shader) { device_.destroyShaderModule(shader); });


		const auto shaderStageCreateInfos = utils::make_array<vk::PipelineShaderStageCreateInfo>(
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main"),
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main")
		);
	}

	Pipeline(Pipeline&& other) noexcept
		: device_(other.device_)
	{
	}

	Pipeline& operator=(Pipeline&& other) noexcept
	{
		return *this;
	}

private:

	std::vector<uint32_t> ReadFile(const wchar_t* path)
	{
		std::ifstream fileStream(path, std::ios::ate | std::ios::binary);

		if (!fileStream.is_open())
			throw std::runtime_error("Shader file not found");

		const size_t fileSize = fileStream.tellg();
		fileStream.seekg(0);

		if (fileSize % sizeof(size_t) != 0)
			throw std::runtime_error("Shader file size is not a multiple of 4"); // meh, it contains hardcoded number

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		fileStream.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		return buffer;
	}

	vk::ShaderModule LoadShaderModule(const wchar_t* path)
	{
		const auto data = ReadFile(path);

		return device_.createShaderModule(
			vk::ShaderModuleCreateInfo().setPCode(data.data()).setCodeSize(data.size() * sizeof(decltype(data)::value_type)));
	}
};
