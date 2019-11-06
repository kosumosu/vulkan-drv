#pragma once

enum class PresentationMode
{
	Immediate,
	VSyncDoubleBuffering,
	RelaxedVSyncDoubleBuffering,
	VSyncTripleBuffering,
};

struct RendererSettings
{
	PresentationMode presentationMode;
};