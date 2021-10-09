#include "stdafx.h"
#include "renderer.h"
#ifdef RE_PLATFORM_WIN64
//#include "GL4.6/renderer_gl.h"
#elif RE_PLATFORM_MACOS
#include "Vulkan/renderer_vk.h"
#else
//#include "GLES3.2/renderer_gles.h"
#endif
//#include "resource.h"
#include "assert.h"
#include "rgfx/mesh.h"
//#include "gui.h"
#include "system.h"
#include "math/vec4.h"
//#include "stb/stretchy_buffer.h"
//#include "hash.h"
//#include <threads.h>

#ifdef VRSYSTEM_OCULUS
#include "OVR_CAPI_GL.h"
#endif

/*
#ifndef RE_PLATFORM_WIN64
#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"

//extern VrApi vrapi;
#endif
*/

#define NUM_X (3)
#define NUM_Y (3)
#define NUM_Z (3)
#define NUM_DRAWS (NUM_X * NUM_Y * NUM_Z)

#define ION_PI (3.14159265359f)
#define ION_180_OVER_PI (180.0f / ION_PI)
#define ION_PI_OVER_180 (ION_PI / 180.0f)

#define DEGREES(x) ((x) * ION_180_OVER_PI)
#define RADIANS(x) ((x) * ION_PI_OVER_180)

//#include "vrsystem.h"

#define USE_SEPARATE_SHADERS_OBJECTS
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

extern inline rgfx_buffer rgfx_createBuffer(const rgfx_bufferDesc* desc);
extern inline GLuint rgfx_getNativeBufferFlags(uint32_t flags);
extern inline rgfx_buffer rgfx_createBuffer(const rgfx_bufferDesc* desc);
extern inline void* rgfx_mapBuffer(rgfx_buffer buffer);
extern inline void* rgfx_mapBufferRange(rgfx_buffer buffer, int64_t offset, int64_t size);
extern inline void rgfx_unmapBuffer(rgfx_buffer buffer);
extern inline void rgfx_bindBuffer(int32_t index, rgfx_buffer buffer);
extern inline void rgfx_bindBufferRange(int32_t index, rgfx_buffer buffer, int64_t offset, int64_t size);

extern inline rgfx_vertexBuffer rgfx_createVertexBuffer(const rgfx_vertexBufferDesc* desc);
extern inline uint32_t rgfx_writeVertexData(rgfx_vertexBuffer vb, size_t sizeInBytes, uint8_t* vertexData);
extern inline uint32_t rgfx_writeIndexData(rgfx_vertexBuffer vb, size_t sizeInBytes, uint8_t* indexData, size_t sourceIndexSize);

extern inline GLenum rgfx_getNativePrimType(rgfx_primType primType);
extern inline GLenum rgfx_getNativeTextureFormat(rgfx_format format);
extern inline rgfx_pass rgfx_createPass(const rgfx_passDesc* desc);

typedef struct rapp_data
{
    mat4x4 worldMatrix;
    mat4x4 viewMatrix;
    mat4x4 projectionMatrix;
}rapp_data;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-pointer-arithmetic"

//static __attribute__((aligned(64))) rgfx_rendererData s_rendererData = {};
static __attribute__((aligned(64))) rapp_data s_appData = {};

#if 0
#ifdef _DEBUG
void openglCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	(void)source; (void)type; (void)id;
	(void)severity; (void)length; (void)userParam;

	if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH)
		printf("%s\n", message);
	if (severity == GL_DEBUG_SEVERITY_HIGH)
	{
		if (type != GL_DEBUG_TYPE_PERFORMANCE)
		{
			printf("High Severity...\n");
		}
		//			abort();
	}
}
#endif
#endif

void rgfx_initialize(const rgfx_initParams* params)
{
    rgfx_initializePlatform(params);

    s_rendererData.cameraTransforms = rgfx_createBuffer(&(rgfx_bufferDesc) {
        .capacity = sizeof(rgfx_cameraTransform) * MAX_CAMERAS,
        .stride = sizeof(rgfx_cameraTransform),
        .flags = kMapWriteBit
    });

    float dt = 0.0f;
    static float angle = 0.0f;
    float sina = 1.5f * sinf(RADIANS(angle));
    float cosa = 1.5f * cosf(RADIANS(angle));
    float radius = 0.5f + (float)(NUM_Z - 1) / 2.0f;
    vec4 eye = { sina * radius, 1.0f, cosa * radius, 1.0f };
    vec4 at = { 0.0f, 1.0f, 0.0f, 1.0f };
    vec4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    s_appData.worldMatrix = mat4x4_lookAtWorld(eye, at, up);
    s_appData.viewMatrix = mat4x4_orthoInverse(s_appData.worldMatrix);
#ifndef DISABLE_CAMERA_SPIN
    angle += 15.0f * dt;
    if (angle >= 360.0f)
        angle -= 360.0f;
#endif
    const float fov = RADIANS(90.0f);
    const float aspectRatio = (float)1080 / (float)1920;
    const float nearZ = 0.1f;
    const float farZ = 100.0f;
    const float focalLength = 1.0f / tanf(fov * 0.5f);

    float left = -nearZ / focalLength;
    float right = nearZ / focalLength;
    float bottom = -aspectRatio * nearZ / focalLength;
    float top = aspectRatio * nearZ / focalLength;

    s_appData.projectionMatrix = mat4x4_frustum(left, right, bottom, top, nearZ, farZ);

#if 0
	memset(&s_rendererData, 0, sizeof(rgfx_rendererData));
	rgfx_iniializeScene();

	rgfx_initializeExtensions(params->extensions);

	s_rendererData.numEyeBuffers = 1;
	if (rvrs_isInitialized())
	{
		memset(s_rendererData.eyeFbInfo, 0, sizeof(rgfx_eyeFbInfo) * kEyeCount);
		s_rendererData.numEyeBuffers = (int32_t)kEyeCount; // VRAPI_FRAME_LAYER_EYE_MAX;

		int32_t numBuffers = params->extensions.multi_view ? 1 : kEyeCount;
		// Create the frame buffers.
		for (int32_t eye = 0; eye < numBuffers; eye++)
		{
			rgfx_createEyeFrameBuffers(eye, params->eyeWidth, params->eyeHeight, kFormatSRGBA8, 4, params->extensions.multi_view);
		}
		s_rendererData.numEyeBuffers = numBuffers;
	}

	GLint numVertexAttribs;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numVertexAttribs);
	s_rendererData.numVertexBuffers = 0;
#ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
#endif

	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);

	const uint32_t bufferSize = 2 * 1024 * 1024;

	// Unskinned Vertex Buffer
	rgfx_vertexBuffer unskinnedVb = { 0 };
	{
		rgfx_vertexElement vertexStreamElements[] =
		{
			{ kFloat, 3, false },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kFloat, 2, false },
		};
		unskinnedVb = rgfx_createVertexBuffer(&(rgfx_vertexBufferDesc) {
			.format = rgfx_registerVertexFormat(STATIC_ARRAY_SIZE(vertexStreamElements), vertexStreamElements),
			.capacity = bufferSize,
			.indexBuffer = rgfx_createBuffer(&(rgfx_bufferDesc) {
				.type = kIndexBuffer,
				.capacity = bufferSize,
				.stride = sizeof(uint16_t),
				.flags = kMapWriteBit | kDynamicStorageBit
			})
		});
	}

	// Skinned Vertex Buffer
	rgfx_vertexBuffer skinnedVb = { 0 };
	{
		rgfx_vertexElement vertexStreamElements[] =
		{
			{ kFloat, 3, false },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kInt2_10_10_10_Rev, 4, true },
			{ kFloat, 2, false },
			{ kSignedInt, 4, false },
			{ kFloat, 4, false },
		};
		skinnedVb = rgfx_createVertexBuffer(&(rgfx_vertexBufferDesc) {
			.format = rgfx_registerVertexFormat(STATIC_ARRAY_SIZE(vertexStreamElements), vertexStreamElements),
			.capacity = bufferSize,
			.indexBuffer = rgfx_createBuffer(&(rgfx_bufferDesc) {
				.type = kIndexBuffer,
				.capacity = bufferSize,
				.stride = sizeof(uint16_t),
				.flags = kMapWriteBit | kDynamicStorageBit
			})
		});
	}

	s_rendererData.cameraTransforms = rgfx_createBuffer(&(rgfx_bufferDesc) {
		.capacity = sizeof(rgfx_cameraTransform) * MAX_CAMERAS,
		.stride = sizeof(rgfx_cameraTransform),
		.flags = kMapWriteBit
	});

	s_rendererData.lights = rgfx_createBuffer(&(rgfx_bufferDesc) {
		.capacity = sizeof(rgfx_light) * 8,
		.stride = sizeof(rgfx_light),
		.flags = kMapWriteBit
	});
#endif
}

void rgfx_initPostEffects(void)
{
#if 0
	s_rendererData.fsQuadPipeline = rgfx_createPipeline(&(rgfx_pipelineDesc) {
		.vertexShader = rgfx_loadShader("shaders/fsquad.vert", kVertexShader, 0),
		.fragmentShader = rgfx_loadShader("shaders/fsquad.frag", kFragmentShader, 0),
	});
#endif
}

rgfx_bufferInfo* rgfx_getVertexBufferInfo(rgfx_vertexBuffer vb)
{
#if 0
	int32_t vbIdx = vb.id - 1;
	assert(vbIdx >= 0);
	rgfx_buffer buffer = s_rendererData.vertexBufferInfo[vbIdx].vertexBuffer;
	int32_t bufferIdx = buffer.id - 1;
	assert(bufferIdx >= 0);
	return &s_rendererData.buffers[bufferIdx];
#endif
	return NULL;
}

rgfx_buffer rgfx_getVertexBufferBuffer(rgfx_vertexBuffer vb, rgfx_bufferType type)
{
#if 0
	int32_t vbIdx = vb.id - 1;
	assert(vbIdx >= 0);
	switch (type)
	{
	case kVertexBuffer:
		return s_rendererData.vertexBufferInfo[vbIdx].vertexBuffer;
		break;
	case kIndexBuffer:
		return s_rendererData.vertexBufferInfo[vbIdx].indexBuffer;
		break;
	default:
		rsys_print("Warning: Invalid buffer type requested.\n");
	};
#endif
	return (rgfx_buffer) { 0 };
}

rgfx_bufferInfo* rgfx_getIndexBufferInfo(rgfx_vertexBuffer vb)
{
#if 0
	int32_t vbIdx = vb.id - 1;
	assert(vbIdx >= 0);
	rgfx_buffer buffer = s_rendererData.vertexBufferInfo[vbIdx].indexBuffer;
	int32_t bufferIdx = buffer.id - 1;
	assert(bufferIdx >= 0);
	return &s_rendererData.buffers[bufferIdx];
#endif
	return NULL;
}

void rgfx_writeCompressedMipImageData(rgfx_texture tex, int32_t mipLevel, rgfx_format format, int32_t width, int32_t height, int32_t imageSize, const void* data)
{
#if 0
	int32_t texIdx = tex.id - 1;
	assert(texIdx >= 0);
	assert(texIdx < MAX_TEXTURES);

	GLenum internalFormat = rgfx_getNativeTextureFormat(format);

	rgfx_textureInfo* texInfo = &s_rendererData.textures[texIdx];
	glBindTexture(texInfo->target, texInfo->name);
	glCompressedTexImage2D(texInfo->target, mipLevel, internalFormat, width, height, 0, imageSize, data);
	glBindTexture(texInfo->target, 0);
#endif
}

void rgfx_writeCompressedTexSubImage3D(rgfx_texture tex, int32_t mipLevel, int32_t xOffset, int32_t yOffset, int32_t zOffset, int32_t width, int32_t height, int32_t depth, rgfx_format format, int32_t imageSize, const void* data)
{
#if 0
	int32_t texIdx = tex.id - 1;
	assert(texIdx >= 0);
	assert(texIdx < MAX_TEXTURES);

	GLenum internalFormat = rgfx_getNativeTextureFormat(format);
	if (s_rendererData.textures[texIdx].internalFormat == 0)
	{
		assert(mipLevel == 0);
		GLuint name = s_rendererData.textures[texIdx].name;
		GLenum target = s_rendererData.textures[texIdx].target;
		glBindTexture(target, name);
		int32_t allocDepth = s_rendererData.textures[texIdx].depth;
		int32_t mipCount = log2(width) + 1;
		switch (target)
		{
		case GL_TEXTURE_2D:
			glTexStorage2D(target, mipCount, internalFormat, width, height);
			break;
		case GL_TEXTURE_2D_ARRAY:
			assert(allocDepth > 0);
			glTexStorage3D(target, mipCount, internalFormat, width, height, allocDepth);
			break;
		default:
			rsys_print("Currently unsupported texture type.\n");
			assert(0);
		}
		s_rendererData.textures[texIdx].width = width;
		s_rendererData.textures[texIdx].height = height;
		s_rendererData.textures[texIdx].internalFormat = internalFormat;
	}

	assert(internalFormat == s_rendererData.textures[texIdx].internalFormat);

	rgfx_textureInfo* texInfo = &s_rendererData.textures[texIdx];
	glBindTexture(texInfo->target, texInfo->name);
	glCompressedTexSubImage3D(texInfo->target, mipLevel, xOffset, yOffset, zOffset, width, height, depth, internalFormat, imageSize, data);
	glBindTexture(texInfo->target, 0);
#endif
}

rgfx_bufferInfo* rgfx_getBufferInfo(rgfx_buffer buffer)
{
#if 0
	int32_t bufferIdx = buffer.id - 1;
	assert(bufferIdx >= 0);
	return &s_rendererData.buffers[bufferIdx];
#endif
	return NULL;
}

GLint g_viewIdLoc = -1;

void rgfx_bindEyeFrameBuffer(int32_t eye)
{
#if 0
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_rendererData.eyeFbInfo[eye].colourBuffers[0]);
#endif
}

rgfx_eyeFbInfo* rgfx_getEyeFrameBuffer(int32_t eye)
{
#if 0
	assert(eye >= 0);
	assert(eye < 2);

	return &s_rendererData.eyeFbInfo[s_rendererData.numEyeBuffers == 1 ? 0 : eye];
#endif
	return NULL;
}

void rgfx_bindTexture(int32_t slot, rgfx_texture tex)
{
#if 0
	int32_t texIdx = tex.id - 1;
	assert(texIdx >= 0);
	assert(texIdx < MAX_TEXTURES);
	assert(slot >= 0);
	assert(slot < 16);
	glActiveTexture(GL_TEXTURE0+slot);
	glBindTexture(s_rendererData.textures[texIdx].target, s_rendererData.textures[texIdx].name);
#endif
}

void rgfx_bindVertexBuffer(rgfx_vertexBuffer vb)
{
#if 0
	int32_t vbIdx = vb.id - 1;
	assert(vbIdx >= 0);
	glBindVertexArray(s_rendererData.vertexBufferInfo[vbIdx].vao);
#endif
}

void rgfx_unbindVertexBuffer()
{
#if 0
	glBindVertexArray(0);
#endif
}

uint32_t rgfx_bindIndexBuffer(rgfx_buffer buffer)
{
#if 0
	int32_t bufferIdx = buffer.id - 1;
	assert(bufferIdx >= 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_rendererData.buffers[bufferIdx].name);
	return s_rendererData.buffers[bufferIdx].writeIndex / s_rendererData.buffers[bufferIdx].stride;
#endif
	return 0;
}

rgfx_vertexFormat rgfx_registerVertexFormat(int32_t numVertexElements, rgfx_vertexElement* vertexElements)
{
#if 0
	assert(s_rendererData.numVertexFormats < MAX_VERTEX_FORMATS);

	//	(void)numVertexElements;
	//	(void)vertexElements;
	//	uint32_t hash;
	//	MurmurHash3_x86_32(vertexElements, sizeof(VertexElement) * numVertexElements, 0xdeadbeef, &hash);
	//	printf("Hash: 0x%08x\n", hash);

	uint32_t vertexFormatHash = hashData(vertexElements, sizeof(rgfx_vertexElement) * numVertexElements); // , 0xdeadbeef, & hash);
	int32_t vertexFormatIndex = rsys_hm_find(&s_rendererData.vertexFormatLookup, vertexFormatHash);
	if (vertexFormatIndex < 0)
	{
		uint8_t elementOffset = 0;
		vertexFormatIndex = s_rendererData.numVertexFormats++;
		for (int32_t i = 0; i < numVertexElements; ++i)
		{
			uint16_t type = vertexElements[i].m_type;
			size_t elementSize = 0;
			char* elementName = NULL;
			switch (type)
			{
			case kFloat:
				elementSize = sizeof(GLfloat) * vertexElements[i].m_size;
				elementName = "GL_FLOAT";
				break;
			case kInt2_10_10_10_Rev:
				elementSize = sizeof(GLint);
				elementName = "GL_INT_2_10_10_10_REV";
				break;
			case kSignedInt:
				elementSize = sizeof(GLint) * vertexElements[i].m_size;
				elementName = "GL_INT";
				break;
			case kUnsignedByte:
				elementSize = sizeof(GLubyte) * vertexElements[i].m_size;
				elementName = "GL_UNSIGNED_BYTE";
				break;
			default:
				assert(0);
			}
			printf("Element: Index = %d, Size = %d, Type = %s, Offset = %d\n", i, vertexElements[i].m_size, elementName, elementOffset);
			elementOffset += elementSize;
			s_rendererData.vertexFormats[vertexFormatIndex].elements[i].m_type = vertexElements[i].m_type;
			s_rendererData.vertexFormats[vertexFormatIndex].elements[i].m_size = vertexElements[i].m_size;
			s_rendererData.vertexFormats[vertexFormatIndex].elements[i].m_normalized = vertexElements[i].m_normalized;
		}
		s_rendererData.vertexFormats[vertexFormatIndex].numElements = numVertexElements;
		s_rendererData.vertexFormats[vertexFormatIndex].stride = elementOffset;
		s_rendererData.vertexFormats[vertexFormatIndex].hash = vertexFormatHash;
		rsys_hm_insert(&s_rendererData.vertexFormatLookup, vertexFormatHash, vertexFormatIndex);

		printf("Vertex stride = %d\n", elementOffset);
	}
	return (rgfx_vertexFormat) { .id = vertexFormatIndex + 1 };
#endif
	return (rgfx_vertexFormat) { 0 };
}

rgfx_vertexFormatInfo* rgfx_getVertexFormatInfo(rgfx_vertexFormat format)
{
#if 0
	int32_t formatIdx = format.id - 1;
	assert(formatIdx >= 0);
	return &s_rendererData.vertexFormats[formatIdx];
#endif
	return NULL;
}

void rgfx_updateTransforms(const rgfx_cameraDesc* cameraDesc)
{
#if 0
	int32_t numCamera = cameraDesc->count > 0 ? cameraDesc->count : 1;

	int32_t idx = s_rendererData.cameraTransforms.id - 1;
	assert(idx >= 0);
	glBindBuffer(GL_UNIFORM_BUFFER, s_rendererData.buffers[idx].name);
	rgfx_cameraTransform* block = (rgfx_cameraTransform*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(rgfx_cameraTransform) * numCamera, s_rendererData.buffers[idx].flags | GL_MAP_INVALIDATE_BUFFER_BIT);

	for (int32_t cam = 0; cam < numCamera; ++cam)
	{
		mat4x4 viewProjMatrx = mat4x4_mul(cameraDesc->camera[cam].projectionMatrix, cameraDesc->camera[cam].viewMatrix);
		block[cam].position = s_rendererData.cameras[cam].position = cameraDesc->camera[cam].position;
		block[cam].viewProjectionMatrix = s_rendererData.cameras[cam].viewProjectionMatrix = viewProjMatrx;
		s_rendererData.cameras[cam].viewMatrix = cameraDesc->camera[cam].viewMatrix;
		s_rendererData.cameras[cam].projectionMatrix = cameraDesc->camera[cam].projectionMatrix;
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif
}

rgfx_texture rgfx_createTextureArray(const char** textureNames, int32_t numTextures)
{
#if 0
	rgfx_texture texArray = rgfx_createTexture(&(rgfx_textureDesc) {
		.depth = numTextures,
		.type = kTexture2DArray,
		.minFilter = kFilterLinearMipmapLinear,
		.magFilter = kFilterLinear,
		.wrapS = kWrapModeRepeat,
		.wrapT = kWrapModeRepeat,			
		.anisoFactor = 1.0f,
	});

	for (int32_t tex = 0; tex < numTextures; ++tex)
	{
		texture_load(textureNames[tex], &(rgfx_textureLoadDesc) {
			.texture = texArray,
			.slice = tex,
		});
	}
	return texArray;
#endif
	return (rgfx_texture) { 0 };
}

rgfx_buffer rgfx_getTransforms()
{
#if 0
	return s_rendererData.cameraTransforms;
#endif
	return (rgfx_buffer) { 0 };
}

rgfx_buffer rgfx_getLightsBuffer()
{
#if 0
	return s_rendererData.lights;
#endif
	return (rgfx_buffer) { 0 };
}

mat4x4 rgfx_getCameraViewMatrix(int32_t cam)
{
#if 0
	assert(cam >= 0);
	assert(cam < MAX_CAMERAS);

	return s_rendererData.cameras[cam].viewMatrix;
#endif
	return mat4x4_identity();
}

mat4x4 rgfx_getCameraProjectionMatrix(int32_t cam)
{
#if 0
	assert(cam >= 0);
	assert(cam < MAX_CAMERAS);

	return s_rendererData.cameras[cam].projectionMatrix;
#endif
	return mat4x4_identity();
}

mat4x4 rgfx_getCameraViewProjMatrix(int32_t cam)
{
#if 0
	assert(cam >= 0);
	assert(cam < MAX_CAMERAS);

	return mat4x4_mul(s_rendererData.cameras[cam].projectionMatrix, s_rendererData.cameras[cam].viewMatrix);
#endif
	return mat4x4_identity();
}

/*
void rgfx_addLight(vec4 position, vec4 color)
{
	int32_t idx = s_rendererData.lights.id - 1;
	assert(idx >= 0);
	glBindBuffer(GL_UNIFORM_BUFFER, s_rendererData.buffers[idx].name);
	rgfx_light* mappedPtr = (rgfx_light*)glMapBufferRange(GL_UNIFORM_BUFFER, sizeof(rgfx_light) * s_rendererData.lightCount++, sizeof(rgfx_light), s_rendererData.buffers[idx].flags);

	if (mappedPtr)
	{
		mappedPtr->position = position;
		mappedPtr->color = color;
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
*/

void rgfx_removeAllMeshInstances()
{
	s_rendererData.numMeshInstances = 0;
}

void rgfx_beginDefaultPass(const rgfx_passAction* passAction, int width, int height)
{
#if 0
	float r = passAction->colors[0].val[0];
	float g = passAction->colors[0].val[1];
	float b = passAction->colors[0].val[2];
	float a = passAction->colors[0].val[3];

	glClearColor(r, g, b, a);
	glClearDepthf(passAction->depth.val);
	GLbitfield clearMask = 0;
	clearMask |= passAction->colors[0].action == kActionClear ? GL_COLOR_BUFFER_BIT : 0;
	clearMask |= passAction->depth.action == kActionClear ? GL_DEPTH_BUFFER_BIT : 0;
	if (clearMask)
	{
		glClear(clearMask);
	}
	glViewport(0, 0, width, height);
	s_rendererData.currentPassInfo.frameBufferId = 0;
	s_rendererData.currentPassInfo.width = width;
	s_rendererData.currentPassInfo.height = height;
	s_rendererData.currentPassInfo.flags = 0;
#endif
}

void rgfx_beginPass(rgfx_pass pass, const rgfx_passAction* passAction)
{
#if 0
	int32_t passIdx = pass.id - 1;
	assert(passIdx >= 0);
	rgfx_passInfo* passInfo = &s_rendererData.passes[passIdx];
	GLuint frameBufferId = passInfo->frameBufferId;
	int32_t width = passInfo->width;
	int32_t height = passInfo->height;
	rgfx_passFlags flags = passInfo->flags;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferId);
#ifdef _DEBUG
	GLuint status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	assert(status == GL_FRAMEBUFFER_COMPLETE); //, "Framebuffer Incomplete\n");
#endif

	float r = passAction->colors[0].val[0];
	float g = passAction->colors[0].val[1];
	float b = passAction->colors[0].val[2];
	float a = passAction->colors[0].val[3];
	glClearColor(r, g, b, a);
	glClearDepthf(passAction->depth.val);

	GLbitfield clearMask = 0;
	clearMask |= passAction->colors[0].action == kActionClear ? GL_COLOR_BUFFER_BIT : 0;
	clearMask |= passAction->depth.action == kActionClear ? GL_DEPTH_BUFFER_BIT : 0;
	if (clearMask)
	{
		glClear(clearMask);
	}
	/*
		if (flags & kPFNeedsResolve)
			glEnable(GL_MULTISAMPLE);
		else
			glDisable(GL_MULTISAMPLE);
	*/
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	s_rendererData.currentPassInfo.frameBufferId = frameBufferId;
	s_rendererData.currentPassInfo.width = width;
	s_rendererData.currentPassInfo.height = height;
	s_rendererData.currentPassInfo.flags = flags;
#endif
}

void rgfx_endPass()
{
#if 0
	if (s_rendererData.currentPassInfo.frameBufferId > 0 && s_rendererData.currentPassInfo.flags & kPFNeedsResolve)
	{
		GLuint frameBufferId = s_rendererData.currentPassInfo.frameBufferId;
		int32_t width = s_rendererData.currentPassInfo.width;
		int32_t height = s_rendererData.currentPassInfo.height;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
#endif
}

void rgfx_bindPipeline(rgfx_pipeline pipeline)
{
#if 0
	int32_t pipeIdx = pipeline.id - 1;
	assert(pipeIdx >= 0);
#ifdef USE_SEPARATE_SHADERS_OBJECTS
	glBindProgramPipeline(s_rendererData.pipelines[pipeIdx].name);
#else
	glUseProgram(s_rendererData.pipelines[pipeIdx].name);
#endif
#endif
}

uint32_t rgfx_getPipelineProgram(rgfx_pipeline pipeline, rgfx_shaderType type)
{
#if 0
	int32_t pipeIdx = pipeline.id - 1;
	assert(pipeIdx >= 0);
	switch (type)
	{
	case kVertexShader:
		return s_rendererData.pipelines[pipeIdx].vertexProgramName;
	case kFragmentShader:
		return s_rendererData.pipelines[pipeIdx].fragmentProgramName;
	default:
		assert(0);
	}
#endif
	return 0;
}


mat4x4* rgfx_mapModelMatricesBuffer(int32_t start, int32_t end)
{
	//	int32_t idx = s_rendererData.instanceMatrices.id;
	//	glBindBuffer(GL_UNIFORM_BUFFER, s_rendererData.buffers[idx].name);
	return NULL; //(mat4x4 *)glMapBufferRange(GL_UNIFORM_BUFFER, start * sizeof(mat4x4), end * sizeof(mat4x4), s_rendererData.buffers[idx].flags);
}

void rgfx_unmapModelMatricesBuffer()
{
	//	glUnmapBuffer(GL_UNIFORM_BUFFER);
	//	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

#if 0
typedef int cmp_t(const void*, const void*);
void insertion_sort(const void *arr, size_t n, size_t elementSize, cmp_t ^cmp_b)
{
	const void* key;
	int i, j;
	for (i = 1; i < n; i++)
	{
		key = (const void*)((uint8_t*)arr + i * elementSize);
		void* keyCpy = alloca(elementSize);
		memcpy(keyCpy, key, sizeof(elementSize));
		j = i - 1;
		while (j >= 0 && cmp_b((const void*)((uint8_t*)arr + j * elementSize), keyCpy) > 0)
		{
			const void* elemJ = (const void*)((uint8_t*)arr + j * elementSize);
			void* elemJ1 = (void*)((uint8_t*)arr + (j+1) * elementSize);
//			arr[j + 1] = arr[j];
			memcpy(elemJ1, elemJ, elementSize);
			j = j - 1;
		}
//		arr[j + 1] = key;
		void* elemJ1 = (void*)((uint8_t*)arr + (j+1) * elementSize);
		memcpy(elemJ1, keyCpy, elementSize);
	}
}
#endif

void rgfx_renderFullscreenQuad(rgfx_texture tex)
{
#if 0
//	glCullFace(GL_BACK);
//	glDepthMask(GL_TRUE);
//	glDisable(GL_CULL_FACE);
	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
#ifdef RE_PLATFORM_WIN64
	glEnable(GL_SAMPLE_ALPHA_TO_ONE);
#endif
//	glEnable(GL_BLEND);

	rgfx_bindVertexBuffer((rgfx_vertexBuffer) { .id = 1 });
	rgfx_bindPipeline(s_rendererData.fsQuadPipeline);
	rgfx_bindTexture(0, tex);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	rgfx_unbindVertexBuffer();
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	glEnable(GL_CULL_FACE);
//	glDisable(GL_BLEND);
#ifdef RE_PLATFORM_WIN64
	glDisable(GL_SAMPLE_ALPHA_TO_ONE);
#endif
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
#endif
}

void checkForCompileErrors(GLuint shader, const char *shaderName) //, GLint shaderType)
{
#if 0
	int32_t error = 0;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &error);
	if (!error)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			infoLen += 128;
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);
			memset(infoLog, 0, infoLen);
			int32_t readLen = 0;
			glGetShaderInfoLog(shader, infoLen, &readLen, infoLog);
			GLint shaderType = 0;
			glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
			if (shaderType == GL_VERTEX_SHADER)
			{
				rsys_print("Error: compiling vertexShader %s: %s\n", shaderName, infoLog);
			}
			else if (shaderType == GL_FRAGMENT_SHADER)
			{
				rsys_print("Error: compiling fragmentShader %s: %s\n", shaderName, infoLog);
			}
			else if (shaderType == GL_COMPUTE_SHADER)
			{
				rsys_print("Error: compiling computeShader\n%s\n", infoLog);
			}
			else
			{
				rsys_print("Error: unknown shader type\n%s\n", infoLog);
			}
			free(infoLog);
		}
	}
#endif
}

rgfx_shader rgfx_loadShader(const char* shaderName, const rgfx_shaderType type, rgfx_materialFlags flags)
{
#if 0
	assert(s_rendererData.numShaders < MAX_SHADERS);
	assert(shaderName != NULL);

	char nameBuffer[1024];
	sprintf(nameBuffer, "%s_%d", shaderName, flags);
	uint32_t hashedShaderName = hashString(nameBuffer);
	int32_t shaderIdx = rsys_hm_find(&s_rendererData.shaderLookup, hashedShaderName);
	if (shaderIdx < 0)
	{
		rsys_file file = file_open(shaderName, "rt");
		if (!file_is_open(file))
		{
			printf("Error: Unable to open file %s\n", shaderName);
			exit(1);
		}
		size_t length = file_length(file);
		char* vs = (char*)malloc(length + 1);
		memset(vs, 0, length + 1);
		file_read(vs, length, file);
		file_close(file);

#ifdef RE_PLATFORM_WIN64
		static const char* programVersion = "#version 460 core\n";
#else
		static const char* programVersion = "#version 320 es\n";
#endif
		const char* compileStrings[20] = { 0 }; // programVersion, NULL, NULL, NULL};
		int32_t compileStringCount;
		GLenum shaderType;

		int32_t stringIndex = 0;
		compileStrings[stringIndex++] = programVersion;
		uint32_t remainingFlags = flags;
		for (int32_t bitIdx = 0; bitIdx < 16 - 1 && remainingFlags; ++bitIdx)
		{
			uint16_t bit = (1 << bitIdx) & flags;
			switch (bit)
			{
			case kMFDiffuseMap:
				compileStrings[stringIndex++] = "#define USE_DIFFUSEMAP\n";
				break;
			case kMFNormalMap:
				compileStrings[stringIndex++] = "#define USE_NORMALMAP\n";
				break;
			case kMFMetallicMap:
				compileStrings[stringIndex++] = "#define USE_METALLICMAP\n";
				break;
			case kMFRoughnessMap:
				compileStrings[stringIndex++] = "#define USE_ROUGHNESSMAP\n";
				break;
			case kMFEmissiveMap:
				compileStrings[stringIndex++] = "#define USE_EMISSIVEMAP\n";
				break;
			case kMFSkinned:
				compileStrings[stringIndex++] = "#define USE_SKINNING\n";
				break;
			case kMFInstancedMaps:
				compileStrings[stringIndex++] = "#define USE_TEXTURE_ARRAY\n";
				break;
			}
			remainingFlags >>= 1;
		}

		switch (type)
		{
		case kVertexShader:
			shaderType = GL_VERTEX_SHADER;
			compileStrings[stringIndex++] = (s_rendererData.extensions.multi_view) ? "#define DISABLE_MULTIVIEW 0\n" : "#define DISABLE_MULTIVIEW 1\n";
			compileStrings[stringIndex++] = vs;
			compileStringCount = stringIndex;
			break;
		case kFragmentShader:
			shaderType = GL_FRAGMENT_SHADER;
			compileStrings[stringIndex++] = vs;
			compileStringCount = stringIndex;
			break;
		default:
			shaderType = GL_INVALID_ENUM;
			compileStringCount = 0;
			assert(0);
		}

#ifdef USE_SEPARATE_SHADERS_OBJECTS
		//	GLuint glShaderName = glCreateShaderProgramv(shaderType, compileStringCount, compileStrings);
		GLuint glShaderName = glCreateShader(shaderType);
		glShaderSource(glShaderName, compileStringCount, compileStrings, NULL);
		glCompileShader(glShaderName);
		checkForCompileErrors(glShaderName, shaderName);
		GLuint glShaderProgramName = glCreateProgram();

		glAttachShader(glShaderProgramName, glShaderName);
		glProgramParameteri(glShaderProgramName, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glLinkProgram(glShaderProgramName);
#else
		GLuint glShaderProgramName = glCreateShader(shaderType);
		glShaderSource(glShaderProgramName, compileStringCount, compileStrings, NULL);
		glCompileShader(glShaderProgramName);
		checkForCompileErrors(glShaderProgramName);
#endif
		free(vs);

		rsys_hm_insert(&s_rendererData.shaderLookup, hashedShaderName, s_rendererData.numShaders);

		shaderIdx = s_rendererData.numShaders++;
		assert(shaderIdx < MAX_SHADERS);
		s_rendererData.shaders[shaderIdx].name = glShaderProgramName;
		s_rendererData.shaders[shaderIdx].type = shaderType;
	}

	return (rgfx_shader) { .id = shaderIdx + 1 };
#endif
	return (rgfx_shader) { 0 };
}

#pragma GCC diagnostic pop

void rgfx_update()
{
    static float angle = 0.0f;

    mat4x4 modelMatrix = mat4x4_rotate(RADIANS(angle), (vec4) { 0.0f, 0.0f, 1.0f, 0.0f });

    vec4 eye = { 0.0f, 0.0f, 2.5f, 1.0f };
    vec4 at = { 0.0f, 0.0f, 0.0f, 1.0f };
    vec4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    s_appData.worldMatrix = mat4x4_lookAtWorld(eye, at, up);
    s_appData.viewMatrix = mat4x4_orthoInverse(s_appData.worldMatrix);

    mat4x4 viewMatrix = s_appData.viewMatrix;
    mat4x4 projectionMatrix = s_appData.projectionMatrix;

    rgfx_updateTransformsTemp(modelMatrix, viewMatrix, projectionMatrix);
        
    angle += 0.25f;
    if (angle > 360.0f)
        angle -= 360.0f;
}
