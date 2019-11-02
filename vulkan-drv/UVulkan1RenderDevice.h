#pragma once

#include "ProperWindows.h"
#include "Engine.h"
#include "UnRender.h"
#include <memory>
#include <optional>
#include <string>

#include <vulkan/vulkan.hpp>

class UVulkan1RenderDevice : public URenderDevice
{
public:
	UVulkan1RenderDevice();

	//UObject glue
#if (UNREALTOURNAMENT || RUNE)
	DECLARE_CLASS(UD3D11RenderDevice, URenderDevice, CLASS_Config, Vulkan1Drv)
#else
DECLARE_CLASS(UVulkan1RenderDevice, URenderDevice, CLASS_Config)
#endif

public:

	/**@name Abstract in parent class */
	//@{	
	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) override;
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen) override;
	void Exit() override;
#if UNREALGOLD || UNREAL
	void Flush();
#else
	void Flush(UBOOL AllowPrecache) override;
#endif
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize) override;
	void Unlock(UBOOL Blit) override;
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet) override;
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span) override;
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span,
	              FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags) override;
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2) override;
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z) override;
	void ClearZ(FSceneNode* Frame) override;
	void PushHit(const BYTE* Data, INT Count) override;
	void PopHit(INT Count, UBOOL bForce) override;
	void GetStats(TCHAR* Result) override;
	void ReadPixels(FColor* Pixels) override;
	//@}

	/**@name Optional but implemented*/
	//@{
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) override;
	void SetSceneNode(FSceneNode* Frame) override;
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags) override;
	void EndFlash() override;
	// ReSharper disable once CppHidingFunction
	void StaticConstructor();
	void InitVulkanInstance();
	//@}

#ifdef RUNE
	/**@name Rune fog*/
//@{
	void DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf);
	void PreDrawGouraud(FSceneNode *Frame, FLOAT FogDistance, FPlane FogColor);
	void PostDrawGouraud(FLOAT FogDistance);
	//@}
#endif

private:
	struct DeviceSearchResult
	{
		vk::PhysicalDevice device;
		size_t renderingQueueFamilyIndex;
		size_t presentationQueueFamilyIndex;
	};

private:
	vk::Instance _instance;
	vk::Device _device;
	size_t _presentationQueueFamilyIndex;
	size_t _renderingQueueFamilyIndex;
	vk::CommandPool _presentationCommandPool;
	vk::CommandPool _renderingCommandPool;

	vk::DebugReportCallbackEXT _debugCallbackHandle;

private:
	static VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData);

	static void DebugPrint(const std::wstring& message);
	std::optional<DeviceSearchResult> FindRequiredPhysicalDevice(const std::vector<vk::PhysicalDevice>& physicalDevices,
	                                                             const vk::SurfaceKHR& presentationSurface) const;
	void InitVirtualDevice(UViewport* InViewport);
	void CreateVirtualDevice(UViewport* InViewport, vk::ResultValueType<vk::Device>::type& device);
};
