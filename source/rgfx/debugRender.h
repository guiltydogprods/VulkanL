#pragma once

#include "math/vec4.h"

typedef struct rgfx_debugColor { uint32_t color; } rgfx_debugColor;

static rgfx_debugColor kDebugColorBlack = { 0xff000000 };
static rgfx_debugColor kDebugColorRed = { 0xff0000ff };
static rgfx_debugColor kDebugColorGreen = { 0xff00ff00 };
static rgfx_debugColor kDebugColorBlue = { 0xffff0000 };
static rgfx_debugColor kDebugColorCyan = { 0xffffff00 };
static rgfx_debugColor kDebugColorMagenta = { 0xffff00ff };
static rgfx_debugColor kDebugColorYellow = { 0xff00ffff };
static rgfx_debugColor kDebugColorWhite = { 0xffffffff };
static rgfx_debugColor kDebugColorGrey = { 0xff7f7f7f };

typedef struct rgfx_debugRenderInitParams
{

}rgfx_debugRenderInitParams;

typedef struct rgfx_debugLine
{
	vec3 p1, p2;
	rgfx_debugColor col1, col2;
	uint32_t pad[2];
}rgfx_debugLine;

void rgfx_debugRenderInitialize(const rgfx_debugRenderInitParams* params);
void rgfx_drawDebugLine(vec3 p1, rgfx_debugColor col1, vec3 p2, rgfx_debugColor col2);
void rgfx_drawDebugLines(int32_t eye, bool bHmdMounted);
