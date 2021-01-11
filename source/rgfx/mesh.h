#pragma once
#include "renderer.h"
#include "math/mat4x4.h"

#ifndef ION_MAKEFOURCC
#define ION_MAKEFOURCC(ch0, ch1, ch2, ch3)											\
					((uint32_t)(uint8_t)(ch0) |	((uint32_t)(uint8_t)(ch1) << 8) |	\
					((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif //defined(ION_MAKEFOURCC)

static int32_t kVersionMajor = 0;
static int32_t kVersionMinor = 2;
static const uint32_t kFourCC_SCNE = ION_MAKEFOURCC('S', 'C', 'N', 'E');	// Scene Chunk FourCC
static const uint32_t kFourCC_TEXT = ION_MAKEFOURCC('T', 'E', 'X', 'T');	// Texture Chunk FourCC
static const uint32_t kFourCC_MATL = ION_MAKEFOURCC('M', 'A', 'T', 'L');	// Material Chunk FourCC
static const uint32_t kFourCC_SKEL = ION_MAKEFOURCC('S', 'K', 'E', 'L');	// Skeleton Chunk FourCC
static const uint32_t kFourCC_LGHT = ION_MAKEFOURCC('L', 'G', 'H', 'T');	// Light Chunk FourCC
static const uint32_t kFourCC_ANIM = ION_MAKEFOURCC('A', 'N', 'I', 'M');	// Anim Chunk FourCC
static const uint32_t kFourCC_MESH = ION_MAKEFOURCC('M', 'E', 'S', 'H');	// Mesh Chunk FourCC
static const uint32_t kFourCC_INST = ION_MAKEFOURCC('I', 'N', 'S', 'T');	// Instance Chunk FourCC
static const uint32_t kFourCC_ENDF = ION_MAKEFOURCC('E', 'N', 'D', 'F');	// End File Chunk FourCC

// Public structures.
typedef struct MeshNode
{
	int32_t	 parentId;
	uint32_t numRenderables;
	uint32_t numChildren;
} MeshNode;

typedef struct Renderable
{
	uint32_t	firstVertex;
	uint32_t	firstIndex;
	uint32_t	indexCount;
	int32_t		materialIndex;
} Renderable;

typedef struct Material
{
	uint32_t			nameHash;
	float				albedo[4];
	float				metallic;
	float				roughness;
	rgfx_texture		albedoMap;
	rgfx_texture		normalMap;
	rgfx_texture		metallicMap;
	rgfx_texture		roughnessMap;
	rgfx_texture		emissiveMap;
	rgfx_texture		ambientMap;
	rgfx_materialFlags	flags;
}Material;

typedef struct Mesh
{
	float		aabbMin[3];
	float		aabbMax[3];
	uint32_t	numNodes;
	uint32_t	numRenderables;
	uint32_t	numBones;
	uint32_t	numFrames;
	int32_t		versionMajor;
	int32_t		versionMinor;

	rsys_hash	materialLookup;
	rsys_hash	boneLookup;

	MeshNode	*hierarchy;
	mat4x4		*transforms;
	Renderable	*renderables;
	Material	*materials;
	mat4x4		*bones;
	mat4x4		*inverseBindBose;
} Mesh;

// Public function prototypes.
void mesh_load(const char* meshName, Mesh* mesh);
