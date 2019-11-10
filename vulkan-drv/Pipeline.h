#pragma once

#include "SelfDestroyable.h"
#include "Bundle.h"

#include "utils.hpp"

#include <vulkan/vulkan.hpp>

#include <boost/noncopyable.hpp>

#include <filesystem>
#include <fstream>
#include <memory>

class Pipeline : boost::noncopyable
{
	vk::Device device_;
	vk::Extent2D viewportExtent_;

	vk::PipelineLayout pipelineLayout_;

	[[nodiscard]] auto GetViewportStateCreateInfo() const
	{
		auto viewport = vk::Viewport()
		                .setWidth(float(viewportExtent_.width))
		                .setHeight(float(viewportExtent_.height))
		                .setMinDepth(0.0f)
		                .setMaxDepth(1.0f);
		vk::Rect2D scissors({0, 0}, viewportExtent_);

		return makeBundle(
			[](auto& viewport, auto& scissors)
			{
				return vk::PipelineViewportStateCreateInfo().setViewportCount(1).setPViewports(&viewport).setScissorCount(1).setPScissors(&scissors);
			},
			std::move(viewport),
			std::move(scissors));
	}

	[[nodiscard]] auto GetShaderStageCreateInfos() const
	{
		// TODO FIX REFERENCE TO TEMPORARY!!!
		auto vertexShaderModule = makeSelfDestroyable(
			LoadShaderModule((std::filesystem::path(DRIVER_DATA_DIRECTORY_NAME) / "shader.vert.spv").wstring().c_str()),
			[&](vk::ShaderModule& shader)
			{
				device_.destroyShaderModule(shader);
			});
		auto fragmentShaderModule = makeSelfDestroyable(
			LoadShaderModule((std::filesystem::path(DRIVER_DATA_DIRECTORY_NAME) / "shader.frag.spv").wstring().c_str()),
			[&](vk::ShaderModule& shader)
			{
				device_.destroyShaderModule(shader);
			});

		return makeBundle(
			[](auto& vertexModule, auto& fragmentModule) -> std::array<vk::PipelineShaderStageCreateInfo, 2>
			{
				return {
					vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexModule, "main"),
					vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentModule, "main")
				};
			},
			std::move(vertexShaderModule),
			std::move(fragmentShaderModule));
	}

	[[nodiscard]] vk::PipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo() const
	{
		return vk::PipelineVertexInputStateCreateInfo();
	}

	[[nodiscard]] vk::PipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo() const
	{
		return vk::PipelineInputAssemblyStateCreateInfo().setPrimitiveRestartEnable(false).setTopology(vk::PrimitiveTopology::eTriangleList);
	}

	[[nodiscard]] vk::PipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo() const
	{
		return vk::PipelineRasterizationStateCreateInfo()
		       .setPolygonMode(vk::PolygonMode::eFill)
		       .setLineWidth(1.0f)
		       .setCullMode(vk::CullModeFlagBits::eBack)
		       .setFrontFace(vk::FrontFace::eClockwise); // TODO: investigate order
	}

	[[nodiscard]] vk::PipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo() const
	{
		return vk::PipelineMultisampleStateCreateInfo().setSampleShadingEnable(false).setMinSampleShading(1.0f).setRasterizationSamples(
			vk::SampleCountFlagBits::e1);
	}

	[[nodiscard]] vk::PipelineDepthStencilStateCreateInfo GetDepthStencilStateCreateInfo() const
	{
		return vk::PipelineDepthStencilStateCreateInfo();
	}

	[[nodiscard]] auto GetColorBlendStateCreateInfo() const
	{
		auto attachmentState = vk::PipelineColorBlendAttachmentState()
		                       .setColorWriteMask(
			                       vk::ColorComponentFlagBits::eA
			                       | vk::ColorComponentFlagBits::eR
			                       | vk::ColorComponentFlagBits::eG
			                       | vk::ColorComponentFlagBits::eB)
		                       .setBlendEnable(true)
		                       .setColorBlendOp(vk::BlendOp::eAdd)
		                       .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		                       .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		                       .setAlphaBlendOp(vk::BlendOp::eAdd)
		                       .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		                       .setDstAlphaBlendFactor(vk::BlendFactor::eZero);

		return makeBundle(
			[](auto& attachmentState)
			{
				return vk::PipelineColorBlendStateCreateInfo().setAttachmentCount(1).setPAttachments(&attachmentState).setLogicOpEnable(false).
				                                               setBlendConstants({});
			},
			std::move(attachmentState)
		);
	}

	[[nodiscard]] auto GetDynamicStateCreateInfo() const
	{
		auto states = utils::make_array<vk::DynamicState>(vk::DynamicState::eBlendConstants, vk::DynamicState::eViewport);

		return makeBundle(
			[](auto& states)
			{
				return vk::PipelineDynamicStateCreateInfo().setDynamicStateCount(states.size()).setPDynamicStates(states.data());
			},
			std::move(states));
	}

	[[nodiscard]] vk::ShaderModule LoadShaderModule(const wchar_t* path) const
	{
		const auto data = ReadFile(path);

		return device_.createShaderModule(
			vk::ShaderModuleCreateInfo().setPCode(data.data()).setCodeSize(data.size() * sizeof(decltype(data)::value_type)));
	}

	[[nodiscard]] std::vector<uint32_t> ReadFile(const wchar_t* path) const
	{
		std::ifstream fileStream(path, std::ios::ate | std::ios::binary);

		if (!fileStream.is_open())
			throw std::runtime_error("Shader file not found");

		const size_t fileSize = size_t(fileStream.tellg());
		fileStream.seekg(0);

		if (fileSize % sizeof(size_t) != 0)
			throw std::runtime_error("Shader file size is not a multiple of 4"); // meh, it contains hardcoded number

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		fileStream.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		return buffer;
	}

	void CreatePipelineLayout()
	{
		pipelineLayout_ = device_.createPipelineLayout(vk::PipelineLayoutCreateInfo());
	}

public:
	explicit Pipeline(vk::Device device, vk::Extent2D viewportExtent)
		: device_(device)
		, viewportExtent_(std::move(viewportExtent))
	{
		const auto shaderStages = GetShaderStageCreateInfos();
		const auto vertexInput = GetVertexInputStateCreateInfo();
		const auto inputAssembly = GetInputAssemblyStateCreateInfo();
		const auto viewport = GetViewportStateCreateInfo();
		const auto rasterization = GetRasterizationStateCreateInfo();
		const auto multisample = GetMultisampleStateCreateInfo();
		const auto depthStencil = GetDepthStencilStateCreateInfo(); // Tutorial passes nullptr for it
		const auto colorBlend = GetColorBlendStateCreateInfo();
		const auto dynamicState = GetDynamicStateCreateInfo();

		CreatePipelineLayout();
	}

	Pipeline(Pipeline&& other) noexcept
		: device_(other.device_)
		, viewportExtent_(other.viewportExtent_)
	{
	}

	Pipeline& operator=(Pipeline&& other) noexcept
	{
		device_ = other.device_;
		viewportExtent_ = other.viewportExtent_;


		return *this;
	}

	~Pipeline()
	{
		device_.destroyPipelineLayout(pipelineLayout_);
		//TODO
	}
};
