// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+
// Inspired by: https://github.com/PCSX2/pcsx2/blob/master/common/CocoaTools.mm

#if !__has_feature(objc_arc)
#error "Compile this with -fobjc-arc"
#endif

#include "CocoaTools.h"
#include "Logger.h"
#include "WindowInfo.h"

#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>

// MARK: - Metal Layers

bool CocoaTools::CreateMetalLayer(WindowInfo* wi)
{
	if (![NSThread isMainThread])
	{
		bool ret;
		dispatch_sync(dispatch_get_main_queue(), [&ret, wi] { ret = CreateMetalLayer(wi); });
		return ret;
	}

	CAMetalLayer* layer = [CAMetalLayer layer];
	if (!layer)
	{
		Logger::error("Failed to create Metal layer.");
		return false;
	}

	NSView* view = (__bridge NSView*)wi->window_handle;
	if (!view)
	{
		Logger::error("Window handle is null in CreateMetalLayer");
		return false;
	}

	[view setWantsLayer:YES];
	[view setLayer:layer];
	[layer setContentsScale:[[[view window] screen] backingScaleFactor]];
	// Store the layer pointer, that way MoltenVK doesn't call [NSView layer] outside the main thread.
	wi->surface_handle = (__bridge_retained void*)layer;
	return true;
}

void CocoaTools::SetDrawableSize(WindowInfo* wi, uint32_t width, uint32_t height)
{
	if (!wi || !wi->surface_handle || width == 0 || height == 0)
		return;

	if (![NSThread isMainThread])
	{
		dispatch_sync(dispatch_get_main_queue(), [wi, width, height] { SetDrawableSize(wi, width, height); });
		return;
	}

	((__bridge CAMetalLayer*)wi->surface_handle).drawableSize = CGSizeMake(width, height);
}

void CocoaTools::DestroyMetalLayer(WindowInfo* wi)
{
	if (!wi)
		return;

	if (![NSThread isMainThread])
	{
		dispatch_sync_f(dispatch_get_main_queue(), wi, [](void* ctx) { DestroyMetalLayer(static_cast<WindowInfo*>(ctx)); });
		return;
	}

	void* raw_layer_handle = wi->surface_handle;
	wi->surface_handle = nullptr;

	if (!raw_layer_handle)
		return;

	// Only release the retained layer reference.
	(void)(__bridge_transfer CAMetalLayer*)raw_layer_handle;
}

std::optional<float> CocoaTools::GetViewRefreshRate(const WindowInfo& wi)
{
	if (![NSThread isMainThread])
	{
		std::optional<float> ret;
		dispatch_sync(dispatch_get_main_queue(), [&ret, wi] { ret = GetViewRefreshRate(wi); });
		return ret;
	}

	std::optional<float> ret;
	NSView* const view = (__bridge NSView*)wi.window_handle;
	if (!view)
		return ret;

	const uint32_t did = [[[[[view window] screen] deviceDescription] valueForKey:@"NSScreenNumber"] unsignedIntValue];
	if (CGDisplayModeRef mode = CGDisplayCopyDisplayMode(did))
	{
		ret = CGDisplayModeGetRefreshRate(mode);
		CGDisplayModeRelease(mode);
	}

	return ret;
}

// MARK: - Directory Services

std::optional<std::string> CocoaTools::GetResourcePath()
{
	@autoreleasepool
	{
		if (NSBundle* bundle = [NSBundle mainBundle])
		{
			NSString* rsrc = [bundle resourcePath];
			NSString* root = [bundle bundlePath];
			if ([rsrc isEqualToString:root])
				rsrc = [rsrc stringByAppendingString:@"/resources"];
			return [rsrc UTF8String];
		}
		return std::nullopt;
	}
}

void CocoaTools::GetWindowInfoFromWindow(WindowInfo* wi, void* cf_window)
{
	if (cf_window)
	{
		NSWindow* window = (__bridge NSWindow*)cf_window;
		float scale = [window backingScaleFactor];
		NSView* view = [window contentView];
		NSRect dims = [view frame];
		wi->type = WindowInfo::Type::MacOS;
		wi->window_handle = (__bridge void*)view;
		wi->surface_width = dims.size.width * scale;
		wi->surface_height = dims.size.height * scale;
		wi->surface_scale = scale;
	}
	else
	{
		wi->type = WindowInfo::Type::Surfaceless;
	}
}
