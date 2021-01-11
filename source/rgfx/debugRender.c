#include "stdafx.h"
#include "debugRender.h"
#include "renderer.h"
#include "stb/stretchy_buffer.h"

#define DISABLE_DEBUG_RENDER

static rgfx_pipeline s_debugRenderPipeline = { 0 };
static rgfx_vertexBuffer s_debugRenderVertexBuffer = { 0 };
static rgfx_debugLine* s_debugLineArray = { 0 };

void rgfx_debugRenderInitialize(const rgfx_debugRenderInitParams* params)
{
#ifdef DISABLE_DEBUG_RENDER
	return;
#endif
	assert(s_debugLineArray == NULL);

	s_debugRenderPipeline = rgfx_createPipeline(&(rgfx_pipelineDesc) {
		.vertexShader = rgfx_loadShader("shaders/debug.vert", kVertexShader, 0),
		.fragmentShader = rgfx_loadShader("shaders/debug.frag", kFragmentShader, 0),
	});

	uint32_t vertexStride = 0;
	{
		rgfx_vertexElement vertexStreamElements[] =
		{
			{ kFloat, 3, false },
			{ kUnsignedByte, 4, true },
		};
		int32_t numElements = STATIC_ARRAY_SIZE(vertexStreamElements);
		rgfx_vertexFormat vertexFormat = rgfx_registerVertexFormat(numElements, vertexStreamElements);
		vertexStride = rgfx_getVertexFormatInfo(vertexFormat)->stride;
		s_debugRenderVertexBuffer = rgfx_createVertexBuffer(&(rgfx_vertexBufferDesc) {
			.format = vertexFormat,
			.capacity = 64 * 1024,
		});
	}
}

void rgfx_drawDebugLine(vec3 p1, rgfx_debugColor col1, vec3 p2, rgfx_debugColor col2)
{
#ifdef DISABLE_DEBUG_RENDER
	return;
#endif
	rgfx_debugLine* lineEntry = sb_add(s_debugLineArray, 1);
	assert(lineEntry != NULL);
	lineEntry->p1 = p1;
	lineEntry->p2 = p2;
	lineEntry->col1 = col1;
	lineEntry->col2 = col2;
}

extern GLint g_viewIdLoc;

void rgfx_drawDebugLines(int32_t eye, bool bHmdMounted)
{
#ifdef DISABLE_DEBUG_RENDER
	return;
#endif
	int32_t numLines = sb_count(s_debugLineArray);
	if (numLines == 0)
		return;

	rgfx_buffer buffer = rgfx_getVertexBufferBuffer(s_debugRenderVertexBuffer, kVertexBuffer);
	float *data = (float*)rgfx_mapBuffer(buffer);

	for (int32_t i = 0; i < numLines; ++i)
	{
		*(data++) = s_debugLineArray[i].p1.x;
		*(data++) = s_debugLineArray[i].p1.y;
		*(data++) = s_debugLineArray[i].p1.z;
		*((rgfx_debugColor*)data++) = s_debugLineArray[i].col1;
		*(data++) = s_debugLineArray[i].p2.x;
		*(data++) = s_debugLineArray[i].p2.y;
		*(data++) = s_debugLineArray[i].p2.z;
		*((rgfx_debugColor*)data++) = s_debugLineArray[i].col2;
	}
	rgfx_unmapBuffer(buffer);

	rgfx_bindVertexBuffer(s_debugRenderVertexBuffer);

//	glDisable(GL_DEPTH_TEST);
//	glDisable(GL_FRAMEBUFFER_SRGB);
	rgfx_bindPipeline(s_debugRenderPipeline);

#ifndef RE_PLATFORM_MACOS
	GLuint program = rgfx_getPipelineProgram(s_debugRenderPipeline, kVertexShader);
	if (g_viewIdLoc >= 0)  // NOTE: will not be present when multiview path is enabled.
	{
		glProgramUniform1i(program, g_viewIdLoc, eye);
	}

	rgfx_bindBuffer(1, rgfx_getTransforms());

	glDrawArrays(GL_LINES, 0, numLines * 2);
#endif
    if (!bHmdMounted || bHmdMounted && eye == 1) {
		sb_free(s_debugLineArray);
		s_debugLineArray = NULL;
	}
	rgfx_unbindVertexBuffer();
//	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_FRAMEBUFFER_SRGB);
}
