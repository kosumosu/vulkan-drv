#include "UVulkan1RenderDevice.h"
#include "resource.h"
#include "utils.hpp"
#include <iostream>
#include <sstream>

using namespace std::string_literals;

//UObject glue
IMPLEMENT_PACKAGE(Vulkan1Drv);
IMPLEMENT_CLASS(UVulkan1RenderDevice);


UVulkan1RenderDevice::UVulkan1RenderDevice()
{
	// Do not remove this constructor because without it, the game crashes when switching to fullscreen mode. Using C++11 "ctor() = default" does not help either.
	// Because in this case the game will call GetClass() that will return nullptr for some unknown reason (the field UObject::Class is not initialized?)
	// Probably a bug in the Microsoft (R) C/C++ Optimizing Compiler Version 19.00.24215.1 for x86 (Visual Studio 2015)
	// or caused by crappy Unreal's custom new operators.
	// Visual studio 2013 toolset does not have such problem.
}


/**
Constructor called by the game when the renderer is first created.
\note Required to compile for Unreal Tournament.
\note Binding settings to the preferences window needs to done here instead of in init() or the game crashes when starting a map if the renderer's been restarted at least once.
*/
void UVulkan1RenderDevice::StaticConstructor()
{
}


void UVulkan1RenderDevice::InitVulkanInstance()
{
	vk::ApplicationInfo appInfo;
	appInfo
		.setApiVersion(VK_API_VERSION_1_0)
		.setPApplicationName("unreal98-Vulkan1Drv")
		.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
		.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));

	constexpr auto extensions = utils::make_array<const char*>(
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if defined(_DEBUG)
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
	);

	constexpr auto layers = utils::make_array<const char*>(
#if defined(_DEBUG)
		"VK_LAYER_LUNARG_standard_validation"
#endif
	);

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo
		.setPApplicationInfo(&appInfo)
		.setPpEnabledExtensionNames(extensions.data())
		.setEnabledExtensionCount(extensions.size())
		.setPpEnabledLayerNames(layers.data())
		.setEnabledLayerCount(layers.size());

	_instance = vk::createInstance(instanceCreateInfo);

	vk::DebugReportCallbackCreateInfoEXT debugCallbackInfo;
	debugCallbackInfo
		.setFlags(vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eInformation)
		.setPfnCallback(VulkanDebugCallback)
		.setPUserData(this);

	_debugCallbackHandle = _instance.createDebugReportCallbackEXT(debugCallbackInfo);
}

VkBool32 VKAPI_CALL UVulkan1RenderDevice::VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                              size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData)
{
	// Select prefix depending on flags passed to the callback
	// Note that multiple flags may be set for a single validation message
	std::wstringstream text;

	text << "[Vulkan.Debug_extension] ";

	// Error that may result in undefined behaviour
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		text << "ERROR:";
	};
	// Warnings may hint at unexpected / non-spec API usage
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		text << "WARNING:";
	};
	// May indicate sub-optimal usage of the API
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		text << "PERFORMANCE:";
	};
	// Informal messages that may become handy during debugging
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		text << "INFO:";
	}
	// Diagnostic info from the Vulkan loader and layers
	// Usually not helpful in terms of API usage, but may help to debug layer and loader problems 
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		text << "DEBUG:";
	}

	// Display message to default output (console if activated)
	text << " [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";

	DebugPrint(text.str());

	return VK_FALSE;
}

void UVulkan1RenderDevice::DebugPrint(const std::wstring& message)
{
	std::wstringstream ss;
	ss << "[Vulkan1Drv] " << message;
	GLog->Log(ss.str().c_str());
}

std::optional<UVulkan1RenderDevice::DeviceSearchResult> UVulkan1RenderDevice::FindRequiredPhysicalDevice(
	const std::vector<vk::PhysicalDevice>& physicalDevices, const vk::SurfaceKHR& presentationSurface) const
{
	for (const auto& physicalDevice : physicalDevices)
	{
		const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
		const auto renderingQueueIt = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const vk::QueueFamilyProperties& props)
		{
			return props.queueFlags & vk::QueueFlagBits::eGraphics;
		});

		if (renderingQueueIt == queueFamilies.end())
			continue;

		bool hasPresentationQueue = false;
		size_t presentationQueueIndex = 0;
		for (size_t i = 0; i < queueFamilies.size(); ++i)
		{
			if (physicalDevice.getSurfaceSupportKHR(i, presentationSurface))
			{
				hasPresentationQueue = true;
				presentationQueueIndex = i;
				break;
			}
		}

		if (!hasPresentationQueue)
			continue;

		const auto presentationQueueIt = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const vk::QueueFamilyProperties& props)
		{
			return props.queueFlags & vk::QueueFlagBits::eGraphics;
		});
		const auto renderingQueueIndex = std::distance(queueFamilies.begin(), renderingQueueIt);

		return DeviceSearchResult{physicalDevice, size_t(renderingQueueIndex), presentationQueueIndex};
	}

	return std::nullopt;
}

void UVulkan1RenderDevice::InitVirtualDevice(UViewport* InViewport)
{
	vk::Win32SurfaceCreateInfoKHR surfaceInfo;
	surfaceInfo
		.setHinstance(GetModuleHandle(nullptr))
		.setHwnd(static_cast<HWND>(InViewport->GetWindow()));


	const auto presentationSurface = _instance.createWin32SurfaceKHR(surfaceInfo);

	const auto physicalDevices = _instance.enumeratePhysicalDevices();
	if (physicalDevices.empty())
		throw std::runtime_error("Vulkan reported no devices.");

	const auto deviceSearchResult = FindRequiredPhysicalDevice(physicalDevices, presentationSurface);

	if (!deviceSearchResult)
		throw std::runtime_error("Can't find supported device (presentation + graphics)");

	const float priority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;

	queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), deviceSearchResult->presentationQueueFamilyIndex, 1, &priority));
	if (deviceSearchResult->presentationQueueFamilyIndex != deviceSearchResult->renderingQueueFamilyIndex)
		queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), deviceSearchResult->presentationQueueFamilyIndex, 1, &priority));

	constexpr auto extensions = utils::make_array<const char *>(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	vk::PhysicalDeviceFeatures features;
	features.setTextureCompressionBC(VK_TRUE);

	vk::DeviceCreateInfo deviceInfo;
	deviceInfo
		.setPpEnabledExtensionNames(&extensions[0])
		.setEnabledExtensionCount(extensions.size())
		.setPQueueCreateInfos(queueInfos.data())
		.setQueueCreateInfoCount(queueInfos.size())
		.setPEnabledFeatures(&features);

	std::wstringstream ss;
	ss << "Found device: \"" << deviceSearchResult->device.getProperties().deviceName << "\"";
	DebugPrint(ss.str());

	_device = deviceSearchResult->device.createDevice(deviceInfo);
	_presentationQueueFamilyIndex = deviceSearchResult->presentationQueueFamilyIndex;
	_renderingQueueFamilyIndex = deviceSearchResult->renderingQueueFamilyIndex;
}

/**
Initialization of renderer.
- Set parent class options. Some of these are settings for the renderer to heed, others control what the game does.
	- URenderDevice::SpanBased; Probably for software renderers.
	- URenderDevice::Fullscreen; Only for Voodoo cards.
	- URenderDevice::SupportsTC; Game sends compressed textures if present.
	- URenderDevice::SupportsDistanceFog; Distance fog. Don't know how this is supposed to be implemented.
	- URenderDevice::SupportsLazyTextures; Renderer loads and unloads texture info when needed (???).
	- URenderDevice::PrefersDeferredLoad; Renderer prefers not to cache textures in advance (???).
	- URenderDevice::ShinySurfaces; Renderer supports detail textures. The game sends them always, so it's meant as a detail setting for the renderer.
	- URenderDevice::Coronas; If enabled, the game draws light coronas.
	- URenderDevice::HighDetailActors; If enabled, game sends more detailed models (???).
	- URenderDevice::VolumetricLighting; If enabled, the game sets fog textures for surfaces if needed.
	- URenderDevice::PrecacheOnFlip; The game will call the PrecacheTexture() function to load textures in advance. Also see Flush().
	- URenderDevice::Viewport; Always set to InViewport.
- Initialize graphics api.
- Resize buffers (convenient to use SetRes() for this).

\param InViewport viewport parameters, can get the window handle.
\param NewX Viewport width.
\param NewY Viewport height.
\param NewColorBytes Color depth.
\param Fullscreen Whether fullscreen mode should be used.
\return 1 if init successful. On 0, game errors out.

\note This renderer ignores color depth.
*/
UBOOL UVulkan1RenderDevice::Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	Viewport = InViewport;
	SpanBased = 0;
	FullscreenOnly = 0;
	SupportsFogMaps = 1;
	SupportsTC = 1;
	SupportsDistanceFog = 0;
	SupportsLazyTextures = 0;

	//Force on detail options as not all games give easy access to these
	Coronas = 1;
#if (!UNREALGOLD)
	DetailTextures = 1;
#endif
	ShinySurfaces = 1;
	HighDetailActors = 1;
	VolumetricLighting = 1;
	//PrecacheOnFlip = 1; //Turned on to immediately recache on init (prevents lack of textures after fullscreen switch)

	//Do some nice compatibility fixing: set processor affinity to single-cpu
	//SetProcessAffinityMask(GetCurrentProcess(), 0x1);

	InitVulkanInstance();

	InitVirtualDevice(InViewport);

	vk::CommandPoolCreateInfo presentationCommandPoolCreateInfo;
	presentationCommandPoolCreateInfo
		.setQueueFamilyIndex(_presentationQueueFamilyIndex);
	_presentationCommandPool = _device.createCommandPool(presentationCommandPoolCreateInfo);

	vk::CommandPoolCreateInfo renderingCommandPoolCreateInfo;
	renderingCommandPoolCreateInfo
		.setQueueFamilyIndex(_renderingQueueFamilyIndex);
	_renderingCommandPool = _device.createCommandPool(presentationCommandPoolCreateInfo);

	return SetRes(NewX, NewY, NewColorBytes, Fullscreen);
}

/**
Resize buffers and viewport.
\return 1 if resize successful. On 0, game errors out.

\note Switching to fullscreen exits and re-initializes the renderer.
\note Fullscreen can have values other than 0 and 1 for some reason.
\note This function MUST call URenderDevice::Viewport->ResizeViewport() or the game will stall.
*/
UBOOL UVulkan1RenderDevice::SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	const UBOOL result = URenderDevice::Viewport->ResizeViewport(Fullscreen ? (BLIT_Fullscreen) : (BLIT_HardwarePaint), NewX, NewY, NewColorBytes);

	return result;
}

/**
Cleanup.
*/
void UVulkan1RenderDevice::Exit()
{
	_instance.destroyDebugReportCallbackEXT(_debugCallbackHandle);
	_instance.destroy();
}

/**
Empty texture cache.
\param AllowPrecache Enabled if the game allows us to precache; respond by setting URenderDevice::PrecacheOnFlip = 1 if wanted. This does make load times longer.
*/
#if UNREALGOLD
void UVulkan1RenderDevice::Flush()
#else
void UVulkan1RenderDevice::Flush(UBOOL AllowPrecache)
#endif
{
	return;
	//If caching is allowed, tell the game to make caching calls (PrecacheTexture() function)
#if (!UNREALGOLD)
	if (AllowPrecache)
		PrecacheOnFlip = 1;
#endif
}

/**
Clear screen and depth buffer, prepare buffers to receive data.
\param FlashScale To do with flash effects, see notes.
\param FlashFog To do with flash effects, see notes.
\param ScreenClear The color with which to clear the screen. Used for Rune fog.
\param RenderLockFlags Signify whether the screen should be cleared. Depth buffer should always be cleared.
\param InHitData Something to do with clipping planes; safe to ignore.
\param InHitSize Something to do with clipping planes; safe to ignore.

\note 'Flash' effects are fullscreen colorization, for example when the player is underwater (blue) or being hit (red).
Depending on the values of the related parameters (see source code) this should be drawn; the games don't always send a blank flash when none should be drawn.
EndFlash() ends this, but other renderers actually save the parameters and start drawing it there (probably so it is drawn with the correct depth).
\note RenderLockFlags aren't always properly set, this results in for example glitching in the Unreal castle flyover, in the wall of the tower with the Nali on it.
*/
void UVulkan1RenderDevice::Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize)
{
}

/**
Finish rendering.
/param Blit Whether the front and back buffers should be swapped.
*/
void UVulkan1RenderDevice::Unlock(UBOOL Blit)
{
}

/**
Complex surfaces are used for map geometry. They consists of facets which in turn consist of polys (triangle fans).
\param Frame The scene. See SetSceneNode().
\param Surface Holds information on the various texture passes and the surface's PolyFlags.
	- PolyFlags contains the correct flags for this surface. See polyflags.h
	- Texture is the diffuse texture.
	- DetailTexture is the nice close-up detail that's modulated with the diffuse texture for walls. It's up to the renderer to only draw these on near surfaces.
	- LightMap is the precalculated map lighting. Should be drawn with a -.5 pan offset.
	- FogMap is precalculated fog. Should be drawn with a -.5 pan offset. Should be added, not modulated. Flags determine if it should be applied, see polyflags.h.
	- MacroTexture is similar to a detail texture but for far away surfaces. Rarely used.
\param Facet Contains coordinates and polygons.
	- MapCoords are used to calculate texture coordinates. Involved. See code.
	- Polys is a linked list of triangle fan arrays; each element is similar to the models used in DrawGouraudPolygon().

\note DetailTexture and FogMap are mutually exclusive.
\note Check if submitted polygons are valid (3 or more points).
*/
void UVulkan1RenderDevice::DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet)
{
}

/**
Gouraud shaded polygons are used for 3D models and surprisingly shadows.
They are sent with a call of this function per triangle fan, worldview transformed and lit. They do have normals and texture coordinates (no panning).
\param Frame The scene. See SetSceneNode().
\param Info The texture for the model. Models only come with diffuse textures.
\param Pts A triangle fan stored as an array. Each element has a normal, light (i.e. color) and fog (color due to being in fog).
\param NumPts Number of verts in fan.
\param PolyFlags Contains the correct flags for this model. See polyflags.h
\param Span Probably for software renderers.

\note Modulated models (i.e. shadows) shouldn't have a color, and fog should only be applied to models with the correct flags for that. The D3D10 renderer handles this in the shader.
\note Check if submitted polygons are valid (3 or more points).
*/
void UVulkan1RenderDevice::DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span)
{
}

/**
Used for 2D UI elements, coronas, etc.
\param Frame The scene. See SetSceneNode().
\param Info The texture for the quad.
\param X X coord in screen space.
\param Y Y coord in screen space.
\param XL Width in pixels
\param YL Height in pixels
\param U Texture U coordinate for left.
\param V Texture V coordinate for top.
\param UL U+UL is coordinate for right.
\param VL V+VL is coordinate for bottom.
\param Span Probably for software renderers.
\param Z coordinate (similar to that of other primitives).
\param Color color
\param Fog fog
\param PolyFlags Contains the correct flags for this tile. See polyflags.h

\note Need to set scene node here otherwise Deus Ex dialogue letterboxes will look wrong; they aren't properly sent to SetSceneNode() it seems.
\note Drawn by converting pixel coordinates to -1,1 ranges in vertex shader and drawing quads with X/Y perspective transform disabled.
The Z coordinate however is transformed and divided by W; then W is set to 1 in the shader to get correct depth and yet preserve X and Y.
Other renderers take the opposite approach and multiply X by RProjZ*Z and Y by RProjZ*Z*aspect so they are preserved and then transform everything.
*/
void UVulkan1RenderDevice::DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL,
                                    class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags)
{
}

/**
For UnrealED.
*/
void UVulkan1RenderDevice::Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2)
{
}

/**
For UnrealED.
*/
void UVulkan1RenderDevice::Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z)
{
}

/**
Clear the depth buffer. Used to draw the skybox behind the rest of the geometry, and weapon in front.
\note It is important that any vertex buffer contents be committed before actually clearing the depth!
*/
void UVulkan1RenderDevice::ClearZ(FSceneNode* Frame)
{
}

/**
Something to do with clipping planes, not needed.
*/
void UVulkan1RenderDevice::PushHit(const BYTE* Data, INT Count)
{
}

/**
Something to do with clipping planes, not needed.
*/
void UVulkan1RenderDevice::PopHit(INT Count, UBOOL bForce)
{
}

/**
Something to do with FPS counters etc, not needed.
*/
void UVulkan1RenderDevice::GetStats(TCHAR* Result)
{
}

/**
Used for screenshots and savegame previews.
\param Pixels An array of 32 bit pixels in which to dump the back buffer.
*/
void UVulkan1RenderDevice::ReadPixels(FColor* Pixels)
{
}

/**
Various command from the game. Can be used to intercept input. First let the parent class handle the command.

\param Cmd The command
	- GetRes Should return a list of resolutions in string form "HxW HxW" etc.
	- Brightness is intercepted here
\param Ar A class to which to log responses using Ar.Log().

\note Deus Ex ignores resolutions it does not like.
*/
UBOOL UVulkan1RenderDevice::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	return URenderDevice::Exec(Cmd, Ar);
}


/**
This optional function can be used to set the frustum and viewport parameters per scene change instead of per drawXXXX() call.
\param Frame Contains various information with which to build frustum and viewport.
\note Standard Z parameters: near 1, far 32760. However, it seems ComplexSurfaces (except water's surface when in it) are at least at Z = ~13; models in DX cut scenes ~7. Can be utilized to gain increased z-buffer precision.
Unreal/UT weapons all seem to fall within ZWeapons: Z<12. Can be used to detect, clear depth (to prevent intersecting world) and move them. Only disadvantage of using increased zNear is that water surfaces the player is bobbing in don't look as good.
The D3D10 renderer moves gouraud polygons and tiles with Z < zNear (or Z < ZWeapons if needed) inside the range, allowing Unreal/UT weapons (after a depth clear) and tiles to be displayed correctly. ComplexSurfaces are not moved as this results in odd looking water surfaces.
*/
void UVulkan1RenderDevice::SetSceneNode(FSceneNode* Frame)
{
}

/**
Store a texture in the renderer-kept texture cache. Only called by the game if URenderDevice::PrecacheOnFlip is 1.
\param Info Texture (meta)data. Includes a CacheID with which to index.
\param PolyFlags Contains the correct flags for this texture. See polyflags.h

\note Already cached textures are skipped, unless it's a dynamic texture, in which case it is updated.
\note Extra care is taken to recache textures that aren't saved as masked, but now have flags indicating they should be (masking is not always properly set).
	as this couldn't be anticipated in advance, the texture needs to be deleted and recreated.
*/
void UVulkan1RenderDevice::PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags)
{
}

/**
Other renderers handle flashes here by saving the related structures; this one does it in Lock().
*/
void UVulkan1RenderDevice::EndFlash()
{
}

#ifdef RUNE
/**
Rune world fog is drawn by clearing the screen in the fog color, clipping the world geometry outside the view distance
and then overlaying alpha blended planes. Unfortunately this function is only called once it's actually time to draw the
fog, as such it's difficult to move this into a shader.

\param Frame The scene. See SetSceneNode().
\param ForSurf Fog plane information. Should be drawn with alpha blending enabled, color alpha = position.z/FogDistance.
\note The pre- and post function for this are meant to set blend state but aren't really needed.
*/
void UVulkan1RenderDevice::DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf)
{
	float mult = 1.0 / FogSurf.FogDistance;
	_d3d->setProjectionMode(D3D::PROJ_NORMAL);

	_d3d->setFlags(PF_AlphaBlend, 0);
	_d3d->setTexture(D3D::PASS_DIFFUSE, NULL);
	_d3d->setTexture(D3D::PASS_LIGHT, NULL);
	_d3d->setTexture(D3D::PASS_DETAIL, NULL);
	_d3d->setTexture(D3D::PASS_FOG, NULL);
	_d3d->setTexture(D3D::PASS_MACRO, NULL);
	for (FSavedPoly* Poly = FogSurf.Polys; Poly; Poly = Poly->Next)
	{
		_d3d->indexTriangleFan(Poly->NumPts);
		for (int i = 0; i < Poly->NumPts; i++)
		{
			D3D::Vertex* v = _d3d->getVertex();
			v->flags = FogSurf.PolyFlags;
			v->Color = *((d3dmath::Vec4*)&FogSurf.FogColor.X);
			v->Pos = *(d3dmath::Vec3*)&Poly->Pts[i]->Point.X;
			v->Color.w = v->Pos.z*mult;
			v->flags = PF_AlphaBlend;
		}
	}
}

/**
Rune object fog is normally drawn using the API's linear fog methods. In the D3D10 case, in the shader.
This function tells us how to configure the fog.

\param Frame The scene. See SetSceneNode().
\param FogDistance The end distance of the fog (start distance is always 0)
\param FogColor The fog's color.
*/
void UVulkan1RenderDevice::PreDrawGouraud(FSceneNode *Frame, FLOAT FogDistance, FPlane FogColor)
{
	if (FogDistance > 0)
	{
		d3dmath::Vec4 *color = ((d3dmath::Vec4*)&FogColor.X);
		_d3d->fog(FogDistance, color);
	}
}

/**
Turn off fogging off.
\param FogDistance Distance with which fog was previously turned on.
*/
void UVulkan1RenderDevice::PostDrawGouraud(FLOAT FogDistance)
{
	if (FogDistance > 0)
	{
		_d3d->fog(0, nullptr);
	}
}
#endif
