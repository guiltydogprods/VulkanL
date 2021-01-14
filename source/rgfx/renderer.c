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

//#include "vrsystem.h"
void createVkInstance();
void createSurface(GLFWwindow* window);
void createDevice();
void createSemaphores();
void createSwapChain();
void createCommandPool();
void createRenderPass();
void createFramebuffers();
void createGraphicsPipeline();
void createDescriptorSet();
void createCommandBuffers();

#define USE_SEPARATE_SHADERS_OBJECTS
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

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

const char* validationLayers[] =
{
	"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool kEnableValidationLayers = false;
#else
const bool kEnableValidationLayers = true;
#endif

const uint64_t  kDefaultFenceTimeout = 100000000000;

typedef struct rgfx_device_vk
{
	VkInstance							vkInstance;
	VkDebugReportCallbackEXT			vkDebugReportCallback;

	VkSurfaceKHR						vkSurface;

	uint32_t							vkPhysicalDeviceCount;
	uint32_t							selectedDevice;
	VkPhysicalDevice*					vkPhysicalDevices;
	VkPhysicalDeviceProperties*			vkPhysicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties*	vkPhysicalDeviceMemoryProperties;
	VkPhysicalDeviceFeatures*			vkPhysicalDeviceFeatures;

	uint32_t							vkQueueFamilyPropertiesCount;
	VkQueueFamilyProperties*			vkQueueFamilyProperties;

	uint32_t							vkGraphicsQueueFamilyIndex;
	uint32_t							vkPresentQueueFamilyIndex;

	VkDevice							vkDevice;

	VkQueue								vkGraphicsQueue;
	VkQueue								vkPresentQueue;

	VkSemaphore							vkImageAvailableSemaphore;
	VkSemaphore							vkRenderingFinishedSemaphore;

	VkExtent2D							vkSwapChainExtent;
	VkFormat							vkSwapChainFormat;
	VkSwapchainKHR						vkSwapChain;
	uint32_t							vkSwapChainImageCount;
    VkImage                             vkSwapChainImages[3];
    VkImageView                         vkSwapChainImageViews[3];
    VkFramebuffer                       vkSwapChainFramebuffers[3];
    
    VkCommandPool                       vkCommandPool;
    
    VkRenderPass                        vkRenderPass;

    VkDescriptorSetLayout               vkDescriptorSetLayout;
    VkPipelineLayout                    vkPipelineLayout;
    VkPipeline                          vkGraphicsPipeline;

    VkBuffer                            vkUniformBuffer;
    VkDeviceMemory                      vkUniformBufferMemory;

    VkImage                             vkTextureImage;
    VkDeviceMemory                      vkTextureImageMemory;
    VkImageView                         vkTextureImageView;
    VkSampler                           vkSampler;

    VkDescriptorPool                    vkDescriptorPool;
    VkDescriptorSet                     vkDescriptorSet;
    
    VkBuffer                            vkVertexBuffer;
    VkDeviceMemory                      vkVertexBufferMemory;
    VkBuffer                            vkIndexBuffer;
    VkDeviceMemory                      vkIndexBufferMemory;

    VkCommandBuffer                     vkCommandBuffers[3];

}rgfx_device_vk;

typedef struct UniformBufferData
{
    mat4x4 transformationMatrix;
}UniformBufferData;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-pointer-arithmetic"

//static __attribute__((aligned(64))) rgfx_rendererData s_rendererData = {};
static __attribute__((aligned(64))) rgfx_device_vk s_device = {};
__attribute__((aligned(64))) rgfx_rendererData s_rendererData = {};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		rsys_print("ERROR: validation layer: %s\n", msg);
	else
		rsys_print("WARNING: validation layer: %s\n", msg);
	return VK_FALSE;
}

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
    createVkInstance();
    createSurface(params->glfwWindow);
    createDevice();
    
    createSemaphores();
    createSwapChain();
    createCommandPool();

    createRenderPass();
    createFramebuffers();
    createGraphicsPipeline();
    createDescriptorSet();
    createCommandBuffers();

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

void createVkInstance()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	uint32_t extensionCount = glfwExtensionCount + (kEnableValidationLayers ? 1 : 0);
	const char** extensions = (const char**)alloca(sizeof(const char*) * extensionCount);
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
	{
		extensions[i] = glfwExtensions[i];
	}

	if (kEnableValidationLayers)
	{
		extensions[glfwExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	}

	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties* availableLayers = (VkLayerProperties*)alloca(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
	bool validationLayersFound = false;
	for (uint32_t i = 0; i < layerCount; ++i)
	{
		if (!strcmp(availableLayers[i].layerName, validationLayers[0]))
		{
			validationLayersFound = true;
			break;
		}
	}

	bool enableValidationLayers = kEnableValidationLayers && validationLayersFound;

	VkResult res = vkCreateInstance(&(VkInstanceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//		.pNext = NULL,
//		.flags = 0,
		.pApplicationInfo = &(VkApplicationInfo) {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
//			.pNext = NULL,
			.pApplicationName = "VulkanL",
			.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
			.pEngineName = "RealityEngine",
			.engineVersion = VK_MAKE_VERSION(0, 1, 0),
			.apiVersion = VK_API_VERSION_1_0,
		},
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensions,
		.enabledLayerCount = enableValidationLayers ? sizeof(validationLayers) / sizeof(const char*) : 0,
		.ppEnabledLayerNames = enableValidationLayers ? validationLayers : NULL
	}, NULL, &s_device.vkInstance);

	if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		rsys_print("cannot find a compatible Vulkan ICD\n");
		exit(-1);
	}
	else if (res)
	{
		rsys_print("unknown error\n");
		exit(-1);
	}

	if (kEnableValidationLayers)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_device.vkInstance, "vkCreateDebugReportCallbackEXT");

		VkResult err = CreateDebugReportCallback(s_device.vkInstance, &(VkDebugReportCallbackCreateInfoEXT) {
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback,
			.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
		}, NULL, &s_device.vkDebugReportCallback);

		if (err != VK_SUCCESS)
		{
			rsys_print("failed to create debug callback\n");
			exit(1);
		}
		else
		{
			rsys_print("created debug callback\n");
		}
	}
	else
	{
		rsys_print("skipped creating debug callback\n");
	}
}

void createSurface(GLFWwindow* window)
{
	VkResult err = glfwCreateWindowSurface(s_device.vkInstance, window, NULL, &s_device.vkSurface);
	if (err)
	{
		rsys_print("window surface creation failed.\n");
		exit(-1);
	}
}

void createDevice(void) //ScopeStack& scope)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(s_device.vkInstance, &physicalDeviceCount, NULL);

	s_device.vkPhysicalDeviceCount = physicalDeviceCount;
	s_device.vkPhysicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
	vkEnumeratePhysicalDevices(s_device.vkInstance, &s_device.vkPhysicalDeviceCount, s_device.vkPhysicalDevices);
	s_device.vkPhysicalDeviceProperties = (VkPhysicalDeviceProperties*)malloc(sizeof(VkPhysicalDeviceProperties) * physicalDeviceCount);
	s_device.vkPhysicalDeviceMemoryProperties = (VkPhysicalDeviceMemoryProperties*)malloc(sizeof(VkPhysicalDeviceMemoryProperties) * physicalDeviceCount);
	s_device.vkPhysicalDeviceFeatures = (VkPhysicalDeviceFeatures*)malloc(sizeof(VkPhysicalDeviceFeatures) * physicalDeviceCount);
	for (uint32_t i = 0; i < physicalDeviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(s_device.vkPhysicalDevices[i], &s_device.vkPhysicalDeviceProperties[i]);
		vkGetPhysicalDeviceMemoryProperties(s_device.vkPhysicalDevices[i], &s_device.vkPhysicalDeviceMemoryProperties[i]);
		vkGetPhysicalDeviceFeatures(s_device.vkPhysicalDevices[i], &s_device.vkPhysicalDeviceFeatures[i]);
	}

#if 0 //def WIN32
	SYSTEM_POWER_STATUS powerStatus = {};
	if (GetSystemPowerStatus(&powerStatus))
	{
		if (powerStatus.ACLineStatus == 0 && m_vkPhysicalDeviceCount > 1 && m_vkPhysicalDeviceProperties[1].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			m_selectedDevice = 1;				// Automatically select Integrated GPU if available when on battery power. 
	}
#endif
#if defined(FORCE_NVIDIA)
	m_selectedDevice = 0;
#elif defined(FORCE_INTEL)
	m_selectedDevice = 1;
#endif

	rsys_print("Selected Device: %s\n", s_device.vkPhysicalDeviceProperties[s_device.selectedDevice].deviceName);

	vkGetPhysicalDeviceQueueFamilyProperties(s_device.vkPhysicalDevices[s_device.selectedDevice], &s_device.vkQueueFamilyPropertiesCount, NULL);
	s_device.vkQueueFamilyProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * s_device.vkQueueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(s_device.vkPhysicalDevices[s_device.selectedDevice], &s_device.vkQueueFamilyPropertiesCount, s_device.vkQueueFamilyProperties);

	bool foundGraphicsQueueFamily = false;
	bool foundPresentQueueFamily = false;

	for (uint32_t i = 0; i < s_device.vkQueueFamilyPropertiesCount; i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(s_device.vkPhysicalDevices[s_device.selectedDevice], i, s_device.vkSurface, &presentSupport);

		if (s_device.vkQueueFamilyProperties[i].queueCount > 0 && s_device.vkQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			s_device.vkGraphicsQueueFamilyIndex = i;
			foundGraphicsQueueFamily = true;

			if (presentSupport)
			{
				s_device.vkPresentQueueFamilyIndex = i;
				foundPresentQueueFamily = true;
				break;
			}
		}

		if (!foundPresentQueueFamily && presentSupport)
		{
			s_device.vkPresentQueueFamilyIndex = i;
			foundPresentQueueFamily = true;
		}
	}

	if (foundGraphicsQueueFamily)
	{
		rsys_print("queue family #%d supports graphics\n", s_device.vkGraphicsQueueFamilyIndex);

		if (foundPresentQueueFamily)
		{
			rsys_print("queue family #%d supports presentation\n", s_device.vkPresentQueueFamilyIndex);
		}
		else
		{
			rsys_print("could not find a valid queue family with present support\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		rsys_print("could not find a valid queue family with graphics support\n");
		exit(EXIT_FAILURE);
	}

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//			.pNext = NULL,
//			.flags = 0,
			.queueFamilyIndex = s_device.vkGraphicsQueueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		},
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//			.pNext = NULL,
//			.flags = 0,
			.queueFamilyIndex = s_device.vkPresentQueueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		}
	};
	const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	VkResult res = vkCreateDevice(s_device.vkPhysicalDevices[s_device.selectedDevice], &(VkDeviceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = NULL,
//		.flags = 0,
		.queueCreateInfoCount = s_device.vkGraphicsQueueFamilyIndex != s_device.vkPresentQueueFamilyIndex ? 2 : 1,
		.pQueueCreateInfos = queueCreateInfo,
//		.enabledLayerCount = 0,
//		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = &deviceExtensions,
		.pEnabledFeatures = &s_device.vkPhysicalDeviceFeatures[s_device.selectedDevice],
	}, NULL, &s_device.vkDevice);
// CLR - Should check for errors here.
	vkGetDeviceQueue(s_device.vkDevice, s_device.vkGraphicsQueueFamilyIndex, 0, &s_device.vkGraphicsQueue);
	vkGetDeviceQueue(s_device.vkDevice, s_device.vkPresentQueueFamilyIndex, 0, &s_device.vkPresentQueue);
	rsys_print("acquired graphics and presentation queues\n");
}

void createSemaphores()
{
	VkSemaphoreCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
//	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(s_device.vkDevice, &createInfo, NULL, &s_device.vkImageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(s_device.vkDevice, &createInfo, NULL, &s_device.vkRenderingFinishedSemaphore) != VK_SUCCESS)
	{
		rsys_print("failed to create semaphores\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		rsys_print("created semaphores\n");
	}
}

void createSwapChain()
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_device.vkPhysicalDevices[s_device.selectedDevice], s_device.vkSurface, &surfaceCapabilities);

	uint32_t supportedSurfaceFormatsCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(s_device.vkPhysicalDevices[s_device.selectedDevice], s_device.vkSurface, &supportedSurfaceFormatsCount, NULL);
	VkSurfaceFormatKHR* supportedSurfaceFormats = (VkSurfaceFormatKHR*)alloca(sizeof(VkSurfaceCapabilitiesKHR) * supportedSurfaceFormatsCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(s_device.vkPhysicalDevices[s_device.selectedDevice], s_device.vkSurface, &supportedSurfaceFormatsCount, supportedSurfaceFormats);

	s_device.vkSwapChainExtent = surfaceCapabilities.currentExtent;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
//        .pNext = NULL,
        .surface = s_device.vkSurface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = supportedSurfaceFormats[0].format,
        .imageExtent.width = s_device.vkSwapChainExtent.width,
        .imageExtent.height = s_device.vkSwapChainExtent.height,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
//        .oldSwapchain = VK_NULL_HANDLE,
        .clipped = true,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
//        .queueFamilyIndexCount = 0,
//        .pQueueFamilyIndices = NULL,
    };
/*
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = s_device.vkSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = supportedSurfaceFormats[0].format;
	swapchainCreateInfo.imageExtent.width = s_device.vkSwapChainExtent.width;
	swapchainCreateInfo.imageExtent.height = s_device.vkSwapChainExtent.height;
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = NULL;
*/
	uint32_t queueFamilyIndices[2] =
	{
		(uint32_t)s_device.vkGraphicsQueueFamilyIndex,
		(uint32_t)s_device.vkPresentQueueFamilyIndex
	};

	if (s_device.vkGraphicsQueueFamilyIndex != s_device.vkPresentQueueFamilyIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	s_device.vkSwapChainFormat = supportedSurfaceFormats[0].format;

	VkResult res = vkCreateSwapchainKHR(s_device.vkDevice, &swapchainCreateInfo, NULL, &s_device.vkSwapChain);

	vkGetSwapchainImagesKHR(s_device.vkDevice, s_device.vkSwapChain, &s_device.vkSwapChainImageCount, NULL);
	VkImage* swapChainImages = (VkImage*)alloca(sizeof(VkImage) * s_device.vkSwapChainImageCount);
	if (vkGetSwapchainImagesKHR(s_device.vkDevice, s_device.vkSwapChain, &s_device.vkSwapChainImageCount, swapChainImages) != VK_SUCCESS)
	{
		rsys_print("failed to acquire swap chain images\n");
		exit(EXIT_FAILURE);
	}

//	if (m_swapChainRenderTargets == nullptr && scope != nullptr)
	{
//		m_swapChainRenderTargets = static_cast<RenderTarget**>(scope->allocate(sizeof(RenderTarget) * m_vkSwapChainImageCount));
        VkFormatProperties properties = {};
        vkGetPhysicalDeviceFormatProperties(s_device.vkPhysicalDevices[s_device.selectedDevice], s_device.vkSwapChainFormat, &properties);
/*
        bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        VkImageUsageFlags usage = bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
        VkImageAspectFlags aspectFlags = 0;
        if (!isDepthFormat(format))
        {
            usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else
        {
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
*/
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//            .image = swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = s_device.vkSwapChainFormat,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };


        for (uint32_t i = 0; i < s_device.vkSwapChainImageCount; ++i)
        {
            createInfo.image = swapChainImages[i];
            s_device.vkSwapChainImages[i] = swapChainImages[i];
            if (vkCreateImageView(s_device.vkDevice, &createInfo, NULL, &s_device.vkSwapChainImageViews[i]) != VK_SUCCESS)
            {
                rsys_print("failed to create image view image.\n");
                exit(1);
            }
/*
            s_device.swapChainRenderTargets[i] = rgfx_createSwapchainRenderTargets cope->newObject<RenderTarget>(*this, swapChainImages[i], s_device.vkSwapChainFormat, VK_SAMPLE_COUNT_1_BIT);
 */
        }
//		m_vkSwapChainFramebuffers = static_cast<VkFramebuffer*>(scope->allocate(sizeof(VkFramebuffer) * m_vkSwapChainImageCount));
//		m_vkCommandBuffers = static_cast<VkCommandBuffer*>(scope->allocate(sizeof(VkCommandBuffer) * m_vkSwapChainImageCount));
	}
//	else
//	{
//		for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
//			m_swapChainRenderTargets[i]->recreate(swapChainImages[i]);
//	}
}

void createCommandPool()
{
    VkResult res = vkCreateCommandPool(s_device.vkDevice, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = s_device.vkPresentQueueFamilyIndex,
    }, NULL, &s_device.vkCommandPool);
    if (res != VK_SUCCESS)
    {
        rsys_print("failed to create command pool\n");
        exit(EXIT_FAILURE);
    }
}

void createRenderPass()
{
    VkAttachmentDescription attachments[2] = {
        {
            .format = s_device.vkSwapChainFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
/*
        {
            .format = s_device.vkDepthBufferFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
*/
    };
/*
    VkAttachmentDescription& colorAttachmentDescription = attachments[0];
    colorAttachmentDescription.format = m_vkSwapChainFormat;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
*/
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
/*
    VkAttachmentDescription& depthAttachmentDescription = attachments[1];
    depthAttachmentDescription.format = m_vkDepthBufferFormat;
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
*/
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;
    subPassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = sizeof(attachments) / sizeof(VkAttachmentDescription);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPassDescription;

    if (vkCreateRenderPass(s_device.vkDevice, &createInfo, NULL, &s_device.vkRenderPass) != VK_SUCCESS)
    {
        rsys_print("failed to create render pass\n");
        exit(1);
    }
    else
    {
        rsys_print("created render pass\n");
    }
}

void createFramebuffers()
{
//    m_vkSwapChainImageViews = new VkImageView [m_vkSwapChainImageCount];
//    m_vkSwapChainFramebuffers = new VkFramebuffer [m_vkSwapChainImageCount];

    // Create an image view for every image in the swap chain
    for (uint32_t i = 0; i < s_device.vkSwapChainImageCount; i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = s_device.vkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = s_device.vkSwapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(s_device.vkDevice, &createInfo, NULL, &s_device.vkSwapChainImageViews[i]) != VK_SUCCESS)
        {
            rsys_print("failed to create image view for swap chain image #%zd\n", i);
            exit(1);
        }
    }

    rsys_print("created image views for swap chain images\n");;

    for (uint32_t i = 0; i < s_device.vkSwapChainImageCount; i++)
    {
        VkImageView attachements[] = { s_device.vkSwapChainImageViews[i] }; //, m_vkDepthBufferView };

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = s_device.vkRenderPass;
        createInfo.attachmentCount = sizeof(attachements) / sizeof(VkImageView);
        createInfo.pAttachments = attachements;    //&m_vkSwapChainImageViews[i];
        createInfo.width = s_device.vkSwapChainExtent.width;
        createInfo.height = s_device.vkSwapChainExtent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(s_device.vkDevice, &createInfo, NULL, &s_device.vkSwapChainFramebuffers[i]) != VK_SUCCESS)
        {
            rsys_print("failed to create framebuffer for swap chain image view #%zd\n", i);
            exit(1);
        }
    }

    rsys_print("created framebuffers for swap chain image views.\n");
}

void createGraphicsPipeline()
{
#if 0
    VkShaderModule vertexShaderModule = createShaderModule("Shaders/vert.spv");
    VkShaderModule fragmentShaderModule = createShaderModule("Shaders/frag.spv");

    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = fragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &m_vkVertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 3;
    vertexInputCreateInfo.pVertexAttributeDescriptions = m_vkVertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_vkSwapChainExtent.width;
    viewport.height = (float)m_vkSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = m_vkSwapChainExtent.width;
    scissor.extent.height = m_vkSwapChainExtent.height;

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    descriptorLayoutCreateInfo.pBindings = bindings;    // &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_vkDevice, &descriptorLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create descriptor layout\n");
        exit(1);
    }
    else
    {
        fprintf(stdout, "created descriptor layout\n");
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &m_vkDescriptorSetLayout;

    if (vkCreatePipelineLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create pipeline layout\n");
        exit(1);
    }
    else
    {
        fprintf(stdout, "created pipeline layout\n");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.layout = m_vkPipelineLayout;
    pipelineCreateInfo.renderPass = m_vkRenderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create graphics pipeline\n");
        exit(1);
    }
    else
    {
        fprintf(stdout, "created graphics pipeline\n");
    }

    vkDestroyShaderModule(m_vkDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_vkDevice, fragmentShaderModule, nullptr);
#endif
}

void createDescriptorSet()
{
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 2;
    createInfo.pPoolSizes = poolSizes;    // &typeCount;
    createInfo.maxSets = 1;

    if (vkCreateDescriptorPool(s_device.vkDevice, &createInfo, NULL, &s_device.vkDescriptorPool) != VK_SUCCESS)
    {
        rsys_print("failed to create descriptor pool\n");
        exit(1);
    }
    else
    {
        rsys_print("created descriptor pool\n");
    }

    // There needs to be one descriptor set per binding point in the shader
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = s_device.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &s_device.vkDescriptorSetLayout;

    if (vkAllocateDescriptorSets(s_device.vkDevice, &allocInfo, &s_device.vkDescriptorSet) != VK_SUCCESS)
    {
        rsys_print("failed to create descriptor set\n");
        exit(1);
    }
    else
    {
        rsys_print("created descriptor set\n");
    }

    // Update descriptor set with uniform binding
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = s_device.vkUniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(UniformBufferData);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = s_device.vkTextureImageView;
    imageInfo.sampler = s_device.vkSampler;

    VkWriteDescriptorSet writeDescriptorSet[2] = {};
    writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet = s_device.vkDescriptorSet;
    writeDescriptorSet[0].dstBinding = 0;
    writeDescriptorSet[0].dstArrayElement = 0;
    writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet[0].descriptorCount = 1;
    writeDescriptorSet[0].pBufferInfo = &descriptorBufferInfo;

    writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet = s_device.vkDescriptorSet;
    writeDescriptorSet[1].dstBinding = 1;
    writeDescriptorSet[1].dstArrayElement = 0;
    writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[1].descriptorCount = 1;
    writeDescriptorSet[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(s_device.vkDevice, 2, writeDescriptorSet, 0, NULL);
}

void createCommandBuffers()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = NULL;
    commandBufferAllocateInfo.commandPool = s_device.vkCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = s_device.vkSwapChainImageCount;

//    s_device.vkCommandBuffers = new VkCommandBuffer[m_vkSwapChainImageCount];
    if (vkAllocateCommandBuffers(s_device.vkDevice, &commandBufferAllocateInfo, s_device.vkCommandBuffers) != VK_SUCCESS)
    {
        rsys_print("failed to create command buffers\n");
        exit(EXIT_FAILURE);
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = NULL;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = NULL;

    // Note: contains value for each subresource range
    VkClearValue clearValues[2];
    clearValues[0].color = (VkClearColorValue) { 0.0f, 0.0f, 0.25f, 1.0f };  // R, G, B, A
    clearValues[1].depthStencil = (VkClearDepthStencilValue) { 1.0f, 0 };    // Depth / Stencil

    VkImageSubresourceRange subResourceRange = {};
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.levelCount = 1;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.layerCount = 1;

    // Record the command buffer for every swap chain image
    for (uint32_t i = 0; i < s_device.vkSwapChainImageCount; i++)
    {
        // Record command buffer
        vkBeginCommandBuffer(s_device.vkCommandBuffers[i], &commandBufferBeginInfo);

        // Change layout of image to be optimal for clearing
        // Note: previous layout doesn't matter, which will likely cause contents to be discarded
        VkImageMemoryBarrier presentToDrawBarrier = {};
        presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentToDrawBarrier.srcAccessMask = 0;
        presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (s_device.vkPresentQueueFamilyIndex == s_device.vkGraphicsQueueFamilyIndex)
        {
            presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        else
        {
            presentToDrawBarrier.srcQueueFamilyIndex = s_device.vkPresentQueueFamilyIndex;
            presentToDrawBarrier.dstQueueFamilyIndex = s_device.vkGraphicsQueueFamilyIndex;
        }
        presentToDrawBarrier.image = s_device.vkSwapChainImages[i];
        presentToDrawBarrier.subresourceRange = subResourceRange;

        vkCmdPipelineBarrier(s_device.vkCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &presentToDrawBarrier);

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = s_device.vkRenderPass;
        renderPassBeginInfo.framebuffer = s_device.vkSwapChainFramebuffers[i];
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent = s_device.vkSwapChainExtent;
        renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(VkClearValue);
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(s_device.vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindDescriptorSets(s_device.vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, s_device.vkPipelineLayout, 0, 1, &s_device.vkDescriptorSet, 0, NULL);

        vkCmdBindPipeline(s_device.vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, s_device.vkGraphicsPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(s_device.vkCommandBuffers[i], 0, 1, &s_device.vkVertexBuffer, &offset);

        vkCmdBindIndexBuffer(s_device.vkCommandBuffers[i], s_device.vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(s_device.vkCommandBuffers[i], 12, 1, 0, 0, 0);

        vkCmdEndRenderPass(s_device.vkCommandBuffers[i]);

        // If present and graphics queue families differ, then another barrier is required
        if (s_device.vkPresentQueueFamilyIndex != s_device.vkGraphicsQueueFamilyIndex)
        {
            VkImageMemoryBarrier drawToPresentBarrier = {};
            drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            drawToPresentBarrier.srcQueueFamilyIndex = s_device.vkGraphicsQueueFamilyIndex;
            drawToPresentBarrier.dstQueueFamilyIndex = s_device.vkPresentQueueFamilyIndex;
            drawToPresentBarrier.image = s_device.vkSwapChainImages[i];
            drawToPresentBarrier.subresourceRange = subResourceRange;

            vkCmdPipelineBarrier(s_device.vkCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &drawToPresentBarrier);
        }

        if (vkEndCommandBuffer(s_device.vkCommandBuffers[i]) != VK_SUCCESS)
        {
            rsys_print("failed to record command buffer\n");
            exit(EXIT_FAILURE);
        }
    }

    rsys_print("recorded command buffers\n");

    vkDestroyPipelineLayout(s_device.vkDevice, s_device.vkPipelineLayout, NULL);
}
