//
//  renderer_vk.c
//  VulkanL
//
//  Created by Claire Rogers on 23/09/2021.
//
#include "stdafx.h"
#include <stdio.h>
#include "rgfx_types_internal_vk.h"

void createVkInstance(void);
void createSurface(GLFWwindow* window);
void createDevice(void);
void createSemaphores(void);
void createSwapChain(void);
void createCommandPool(void);
void createVertexBuffer(void);
void createUniformBuffer(void);
void createTexture(const char* filename);
void createRenderPass(void);
void createFramebuffers(void);
void createGraphicsPipeline(void);
void createDescriptorSet(void);
void createCommandBuffers(void);

VkShaderModule createShaderModule(const char *filename);
VkBool32 getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags  properties, uint32_t* typeIndex);

VkCommandBuffer beginSingleUseCommandBuffer(void);
void endSingleUseCommandBuffer(VkCommandBuffer commandBuffer);
void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange* subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout);
void copyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);

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
/*
typedef struct rgfx_device_vk
{
    VkInstance                            vkInstance;
    VkDebugReportCallbackEXT            vkDebugReportCallback;

    VkSurfaceKHR                        vkSurface;

    uint32_t                            vkPhysicalDeviceCount;
    uint32_t                            selectedDevice;
    VkPhysicalDevice*                    vkPhysicalDevices;
    VkPhysicalDeviceProperties*            vkPhysicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties*    vkPhysicalDeviceMemoryProperties;
    VkPhysicalDeviceFeatures*            vkPhysicalDeviceFeatures;

    uint32_t                            vkQueueFamilyPropertiesCount;
    VkQueueFamilyProperties*            vkQueueFamilyProperties;

    uint32_t                            vkGraphicsQueueFamilyIndex;
    uint32_t                            vkPresentQueueFamilyIndex;

    VkDevice                            vkDevice;

    VkQueue                                vkGraphicsQueue;
    VkQueue                                vkPresentQueue;

    VkSemaphore                            vkImageAvailableSemaphore;
    VkSemaphore                            vkRenderingFinishedSemaphore;

    VkExtent2D                            vkSwapChainExtent;
    VkFormat                            vkSwapChainFormat;
    VkSwapchainKHR                        vkSwapChain;
    uint32_t                            vkSwapChainImageCount;
    VkImage                             vkSwapChainImages[3];
    VkImageView                         vkSwapChainImageViews[3];
    VkFramebuffer                       vkSwapChainFramebuffers[3];
    
    VkCommandPool                       vkCommandPool;
    
    VkRenderPass                        vkRenderPass;

    VkVertexInputBindingDescription     vkVertexBindingDescription;
    uint32_t                            vkVertexAttributeDescriptionCount;
    VkVertexInputAttributeDescription   *vkVertexAttributeDescriptions;

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

typedef struct rgfx_gpu_mem_block_vk { int32_t id; } rgfx_gpu_mem_block_vk;

typedef struct rgfx_gpu_mem_block_info_vk
{
//    GPUMemoryBlock(VkDevice device, uint32_t size, uint32_t typeIndex);
//    GPUMemoryBlock() {};
//    ~GPUMemoryBlock();

//    GPUMemAllocInfo& allocate(ScopeStack& scope, uint32_t size, uint32_t alignment, uint32_t optimal);

    VkDevice m_vkDevice;
    VkDeviceMemory m_memory;
    uint32_t m_size;
    uint32_t m_offset;
    uint32_t m_typeIndex;
    uint32_t m_optimal;
}rgfx_gpu_mem_block_info_vk;

typedef struct rgfx_gpu_alloc_info_vk
{
    rgfx_gpu_mem_block_vk memoryBlock;
    uint32_t size;
    uint32_t offset;
}rgfx_gpu_alloc_info_vk;
*/

typedef struct Vertex
{
    float pos[3];
    float color[3];
    float texCoord[2];
}Vertex;

typedef struct UniformBufferData
{
    mat4x4 transformationMatrix;
}UniformBufferData;

__attribute__((aligned(64))) rgfx_device_vk s_device = {};
__attribute__((aligned(64))) rgfx_rendererData s_rendererData = {};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        rsys_print("ERROR: validation layer: %s\n", msg);
    else
        rsys_print("WARNING: validation layer: %s\n", msg);
    return VK_FALSE;
}

void rgfx_initializePlatform(const rgfx_initParams* params)
{
    createVkInstance();
    createSurface(params->glfwWindow);
    createDevice();
    
    createSemaphores();
    createSwapChain();
    createCommandPool();

    createVertexBuffer();
    createUniformBuffer();

    createTexture("assets/copper-rock1-albedo.dds");

    createRenderPass();
    createFramebuffers();
    createGraphicsPipeline();
    createDescriptorSet();
    createCommandBuffers();
}

void rgfx_updateTransformsTemp(mat4x4 modelMatrix, mat4x4 viewMatrix, mat4x4 projectionMatrix)
{
    void *data;
    vkMapMemory(s_device.vkDevice, s_device.vkUniformBufferMemory, 0, sizeof(UniformBufferData), 0, &data);
    UniformBufferData *uboData = (UniformBufferData *)data;
    uboData->transformationMatrix = mat4x4_mul(projectionMatrix, mat4x4_mul(viewMatrix, modelMatrix));
    vkUnmapMemory(s_device.vkDevice, s_device.vkUniformBufferMemory);
}

void rgfx_render()
{
    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(s_device.vkDevice, s_device.vkSwapChain, UINT64_MAX, s_device.vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    {
        fprintf(stderr, "failed to acquire image\n");
//        exit(EXIT_FAILURE);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &s_device.vkImageAvailableSemaphore;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &s_device.vkRenderingFinishedSemaphore;

    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &s_device.vkCommandBuffers[imageIndex];

    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(s_device.vkDevice, &fenceCreateInfo, NULL, &fence) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create fence.\n");
    }
    res = vkQueueSubmit(s_device.vkPresentQueue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "failed to submit draw command buffer\n");
//        exit(EXIT_FAILURE);
    }

    if (vkWaitForFences(s_device.vkDevice, 1, &fence, VK_TRUE, kDefaultFenceTimeout) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to wait on fence.\n");
    }

    vkDestroyFence(s_device.vkDevice, fence, NULL);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &s_device.vkRenderingFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &s_device.vkSwapChain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(s_device.vkPresentQueue, &presentInfo);

    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "failed to submit present command buffer\n");
//        exit(EXIT_FAILURE);
    }
}

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
//        .pNext = NULL,
//        .flags = 0,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
//            .pNext = NULL,
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
            m_selectedDevice = 1;                // Automatically select Integrated GPU if available when on battery power.
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
//            .pNext = NULL,
//            .flags = 0,
            .queueFamilyIndex = s_device.vkGraphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//            .pNext = NULL,
//            .flags = 0,
            .queueFamilyIndex = s_device.vkPresentQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        }
    };
    const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkResult res = vkCreateDevice(s_device.vkPhysicalDevices[s_device.selectedDevice], &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//        .pNext = NULL,
//        .flags = 0,
        .queueCreateInfoCount = s_device.vkGraphicsQueueFamilyIndex != s_device.vkPresentQueueFamilyIndex ? 2 : 1,
        .pQueueCreateInfos = queueCreateInfo,
//        .enabledLayerCount = 0,
//        .ppEnabledLayerNames = NULL,
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
//    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

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

//    if (m_swapChainRenderTargets == nullptr && scope != nullptr)
    {
//        m_swapChainRenderTargets = static_cast<RenderTarget**>(scope->allocate(sizeof(RenderTarget) * m_vkSwapChainImageCount));
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
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
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
//        m_vkSwapChainFramebuffers = static_cast<VkFramebuffer*>(scope->allocate(sizeof(VkFramebuffer) * m_vkSwapChainImageCount));
//        m_vkCommandBuffers = static_cast<VkCommandBuffer*>(scope->allocate(sizeof(VkCommandBuffer) * m_vkSwapChainImageCount));
    }
//    else
//    {
//        for (uint32_t i = 0; i < m_vkSwapChainImageCount; ++i)
//            m_swapChainRenderTargets[i]->recreate(swapChainImages[i]);
//    }
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

#define NEW_VERTS
void createVertexBuffer()
{
    Vertex vertices[] =
    {
#ifndef NEW_VERTS
        { { -1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
#else
        { { -1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
#endif
//        { { -2.0f, -1.0f, -1.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
//        { { -2.0f, 1.0f, -1.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
//        { { 0.0f, 1.0f, -1.5f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
//        { { 0.0f, -1.0f, -1.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },

    };
    uint32_t verticesSize = sizeof(vertices);

#ifndef NEW_VERTS
    uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
#else
    uint32_t indices[] = { 0, 1, 2, 2, 1, 3 }; //2, 3, 0 }; //, 4, 5, 6, 4, 6, 7 };
#endif
    uint32_t indicesSize = sizeof(indices);

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    void *data = NULL;

    struct StagingBuffer
    {
        VkDeviceMemory    memory;
        VkBuffer        buffer;
    };

    struct
    {
        struct StagingBuffer vertices;
        struct StagingBuffer indices;
    }stagingBuffers;

    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = verticesSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(s_device.vkDevice, &vertexBufferInfo, NULL, &stagingBuffers.vertices.buffer);

    vkGetBufferMemoryRequirements(s_device.vkDevice, stagingBuffers.vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(s_device.vkDevice, &memAlloc, NULL, &stagingBuffers.vertices.memory);

    vkMapMemory(s_device.vkDevice, stagingBuffers.vertices.memory, 0, verticesSize, 0, &data);
    memcpy(data, vertices, verticesSize);
    vkUnmapMemory(s_device.vkDevice, stagingBuffers.vertices.memory);
    vkBindBufferMemory(s_device.vkDevice, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(s_device.vkDevice, &vertexBufferInfo, NULL, &s_device.vkVertexBuffer);
    vkGetBufferMemoryRequirements(s_device.vkDevice, s_device.vkVertexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(s_device.vkDevice, &memAlloc, NULL, &s_device.vkVertexBufferMemory);
    vkBindBufferMemory(s_device.vkDevice, s_device.vkVertexBuffer, s_device.vkVertexBufferMemory, 0);

    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indicesSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(s_device.vkDevice, &indexBufferInfo, NULL, &stagingBuffers.indices.buffer);
    vkGetBufferMemoryRequirements(s_device.vkDevice, stagingBuffers.indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(s_device.vkDevice, &memAlloc, NULL, &stagingBuffers.indices.memory);
    vkMapMemory(s_device.vkDevice, stagingBuffers.indices.memory, 0, indicesSize, 0, &data);
    memcpy(data, indices, indicesSize);
    vkUnmapMemory(s_device.vkDevice, stagingBuffers.indices.memory);
    vkBindBufferMemory(s_device.vkDevice, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(s_device.vkDevice, &indexBufferInfo, NULL, &s_device.vkIndexBuffer);
    vkGetBufferMemoryRequirements(s_device.vkDevice, s_device.vkIndexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(s_device.vkDevice, &memAlloc, NULL, &s_device.vkIndexBufferMemory);
    vkBindBufferMemory(s_device.vkDevice, s_device.vkIndexBuffer, s_device.vkIndexBufferMemory, 0);

    VkCommandBuffer copyCommandBuffer = beginSingleUseCommandBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.vertices.buffer, s_device.vkVertexBuffer, 1, &copyRegion);
    copyRegion.size = indicesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.indices.buffer, s_device.vkIndexBuffer, 1, &copyRegion);

    endSingleUseCommandBuffer(copyCommandBuffer);

    vkDestroyBuffer(s_device.vkDevice, stagingBuffers.vertices.buffer, NULL);
    vkFreeMemory(s_device.vkDevice, stagingBuffers.vertices.memory, NULL);
    vkDestroyBuffer(s_device.vkDevice, stagingBuffers.indices.buffer, NULL);
    vkFreeMemory(s_device.vkDevice, stagingBuffers.indices.memory, NULL);

    fprintf(stdout, "set up vertex and index buffers\n");

    s_device.vkVertexBindingDescription.binding = 0;
    s_device.vkVertexBindingDescription.stride = sizeof(Vertex);
    s_device.vkVertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    s_device.vkVertexAttributeDescriptionCount = 3;
    s_device.vkVertexAttributeDescriptions = malloc(sizeof(VkVertexInputAttributeDescription) * s_device.vkVertexAttributeDescriptionCount);
    s_device.vkVertexAttributeDescriptions[0].binding = 0;
    s_device.vkVertexAttributeDescriptions[0].location = 0;
    s_device.vkVertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    s_device.vkVertexAttributeDescriptions[0].offset = 0;

    s_device.vkVertexAttributeDescriptions[1].binding = 0;
    s_device.vkVertexAttributeDescriptions[1].location = 1;
    s_device.vkVertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    s_device.vkVertexAttributeDescriptions[1].offset = sizeof(float) * 3;

    s_device.vkVertexAttributeDescriptions[2].binding = 0;
    s_device.vkVertexAttributeDescriptions[2].location = 2;
    s_device.vkVertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    s_device.vkVertexAttributeDescriptions[2].offset = sizeof(float) * 6;
}

void createUniformBuffer()
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(UniformBufferData);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    vkCreateBuffer(s_device.vkDevice, &bufferInfo, NULL, &s_device.vkUniformBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(s_device.vkDevice, s_device.vkUniformBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);

    vkAllocateMemory(s_device.vkDevice, &allocInfo, NULL, &s_device.vkUniformBufferMemory);
    vkBindBufferMemory(s_device.vkDevice, s_device.vkUniformBuffer, s_device.vkUniformBufferMemory, 0);
}

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)

#define FOURCC_DXT1    MAKEFOURCC('D', 'X', 'T', '1')
#define FOURCC_DXT3    MAKEFOURCC('D', 'X', 'T', '3')
#define FOURCC_DXT5    MAKEFOURCC('D', 'X', 'T', '5')

void createTexture(const char* filename)
{
    FILE *fptr = fopen(filename, "rb");

    fseek(fptr, 0L, SEEK_END);
    uint64_t len = (uint64_t)ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);
    uint8_t* buffer = (uint8_t*)malloc(len);
    fread(buffer, len, 1, fptr);
    fclose(fptr);

    const char* ptr = (const char*)buffer;
    if (strncmp(ptr, "DDS ", 4) != 0)
    {
        fprintf(stdout, "File is not in .dds format.\n");
    }

    uint8_t *header = buffer + 4;
    // get the surface desc
    uint32_t headerSize = *(uint32_t *)&(header[0]);
    uint32_t texHeight = *(uint32_t *)&(header[8]);
    uint32_t texWidth = *(uint32_t *)&(header[12]);
    uint32_t linearSize = *(uint32_t *)&(header[16]);
    uint32_t mipMapCount = *(uint32_t *)&(header[24]);
    uint32_t fourCC = *(uint32_t *)&(header[80]);

    uint32_t components = (fourCC == FOURCC_DXT1) ? 3 : 4;

    VkFormat format;
    switch (fourCC)
    {
    case FOURCC_DXT1:
        format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        break;
    case FOURCC_DXT3:
        format = VK_FORMAT_BC3_UNORM_BLOCK;
        break;
    case FOURCC_DXT5:
        format = VK_FORMAT_BC5_UNORM_BLOCK;
        break;
    default:
        return;
    }

    uint8_t *srcData = header + headerSize;

    uint32_t blockSize = (format == VK_FORMAT_BC1_RGB_UNORM_BLOCK || format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) ? 8 : 16;
    uint64_t size = len - 128;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = size;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    if (vkCreateBuffer(s_device.vkDevice, &bufferCreateInfo, NULL, &stagingBuffer) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create staging buffer for %s\n", filename);
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(s_device.vkDevice, stagingBuffer, &memRequirements);

    VkFormatProperties formatProperties = {};
    vkGetPhysicalDeviceFormatProperties(s_device.vkPhysicalDevices[0], format, &formatProperties);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);

    if (vkAllocateMemory(s_device.vkDevice, &allocInfo, NULL, &stagingBufferMemory) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to allocate memory for staging image\n");
        exit(1);
    }

    if (vkBindBufferMemory(s_device.vkDevice, stagingBuffer, stagingBufferMemory, 0) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to bind memory to staging buffer %s\n", filename);
        exit(1);
    }
    
    VkDeviceSize imageSize = memRequirements.size;

    uint8_t* data = NULL;
    if (vkMapMemory(s_device.vkDevice, stagingBufferMemory, 0, imageSize, 0, (void**)&data) == VK_SUCCESS)
    {
        memcpy(data, srcData, size);
        vkUnmapMemory(s_device.vkDevice, stagingBufferMemory);
    }
    free(buffer);

    // Setup buffer copy regions for each mip level
    VkBufferImageCopy* bufferCopyRegions = (VkBufferImageCopy*)alloca(sizeof(VkBufferImageCopy) * mipMapCount);
    memset(bufferCopyRegions, 0, sizeof(VkBufferImageCopy) * mipMapCount);
    uint32_t offset = 0;
    uint32_t mipWidth = texWidth;
    uint32_t mipHeight = texHeight;
    for (uint32_t i = 0; i < mipMapCount && (mipWidth || mipHeight); i++)
    {
        uint32_t regionSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;

        VkBufferImageCopy* bufferCopyRegion = &bufferCopyRegions[i];
        bufferCopyRegion->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion->imageSubresource.mipLevel = i;
        bufferCopyRegion->imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion->imageSubresource.layerCount = 1;
        bufferCopyRegion->imageExtent.width = mipWidth;
        bufferCopyRegion->imageExtent.height = mipHeight;
        bufferCopyRegion->imageExtent.depth = 1;
        bufferCopyRegion->bufferOffset = offset;
        offset += regionSize > 8 ? regionSize : 8;
        mipWidth /= 2;
        mipHeight /= 2;
    }

    VkImageCreateInfo  imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = NULL;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = texWidth;
    imageCreateInfo.extent.height = texHeight;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipMapCount;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = NULL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(s_device.vkDevice, &imageCreateInfo, NULL, &s_device.vkTextureImage) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create image for %s\n", filename);
        exit(1);
    }

    vkGetImageMemoryRequirements(s_device.vkDevice, s_device.vkTextureImage, &memRequirements);

    VkMemoryAllocateInfo dstAllocInfo = {};
    dstAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    dstAllocInfo.allocationSize = memRequirements.size;
    getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &dstAllocInfo.memoryTypeIndex);

    if (vkAllocateMemory(s_device.vkDevice, &dstAllocInfo, NULL, &s_device.vkTextureImageMemory) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to allocate memory for image\n");
        exit(1);
    }

    if (vkBindImageMemory(s_device.vkDevice, s_device.vkTextureImage, s_device.vkTextureImageMemory, 0) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to bind memory to image\n");
        exit(1);
    }

    VkCommandBuffer commandBuffer = beginSingleUseCommandBuffer();

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipMapCount;
    subresourceRange.layerCount = 1;

    transitionImageLayout(commandBuffer, s_device.vkTextureImage, &subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, s_device.vkTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipMapCount, bufferCopyRegions);
    transitionImageLayout(commandBuffer, s_device.vkTextureImage, &subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommandBuffer(commandBuffer);

    vkDestroyBuffer(s_device.vkDevice, stagingBuffer, NULL);
    vkFreeMemory(s_device.vkDevice, stagingBufferMemory, NULL);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = s_device.vkTextureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange = subresourceRange;
    if (vkCreateImageView(s_device.vkDevice, &viewInfo, NULL, &s_device.vkTextureImageView) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create image view\n");
        exit(EXIT_FAILURE);
    }

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float)mipMapCount;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 8;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    if (vkCreateSampler(s_device.vkDevice, &samplerInfo, NULL, &s_device.vkSampler) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create sampler.\n");
        exit(EXIT_FAILURE);
    }
}

void createRenderPass()
{
    VkAttachmentDescription attachments[1] = {
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
//    VkAttachmentReference depthAttachmentReference = {};
//    depthAttachmentReference.attachment = 1;
//    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.pColorAttachments = &colorAttachmentReference;
//    subPassDescription.pDepthStencilAttachment = &depthAttachmentReference;

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
    VkShaderModule vertexShaderModule = createShaderModule("Shaders/shader.vert.spv");
    VkShaderModule fragmentShaderModule = createShaderModule("Shaders/shader.frag.spv");

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
    vertexInputCreateInfo.pVertexBindingDescriptions = &s_device.vkVertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 3;
    vertexInputCreateInfo.pVertexAttributeDescriptions = s_device.vkVertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)s_device.vkSwapChainExtent.width;
    viewport.height = (float)s_device.vkSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = s_device.vkSwapChainExtent.width;
    scissor.extent.height = s_device.vkSwapChainExtent.height;

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
    samplerLayoutBinding.pImmutableSamplers = NULL;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    descriptorLayoutCreateInfo.pBindings = bindings;    // &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(s_device.vkDevice, &descriptorLayoutCreateInfo, NULL, &s_device.vkDescriptorSetLayout) != VK_SUCCESS)
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
    layoutCreateInfo.pSetLayouts = &s_device.vkDescriptorSetLayout;

    if (vkCreatePipelineLayout(s_device.vkDevice, &layoutCreateInfo, NULL, &s_device.vkPipelineLayout) != VK_SUCCESS)
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
    pipelineCreateInfo.layout = s_device.vkPipelineLayout;
    pipelineCreateInfo.renderPass = s_device.vkRenderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(s_device.vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &s_device.vkGraphicsPipeline) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create graphics pipeline\n");
        exit(1);
    }
    else
    {
        fprintf(stdout, "created graphics pipeline\n");
    }

    vkDestroyShaderModule(s_device.vkDevice, vertexShaderModule, NULL);
    vkDestroyShaderModule(s_device.vkDevice, fragmentShaderModule, NULL);
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
        renderPassBeginInfo.clearValueCount = 1; //sizeof(clearValues) / sizeof(VkClearValue);
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(s_device.vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindDescriptorSets(s_device.vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, s_device.vkPipelineLayout, 0, 1, &s_device.vkDescriptorSet, 0, NULL);

        vkCmdBindPipeline(s_device.vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, s_device.vkGraphicsPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(s_device.vkCommandBuffers[i], 0, 1, &s_device.vkVertexBuffer, &offset);

        vkCmdBindIndexBuffer(s_device.vkCommandBuffers[i], s_device.vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(s_device.vkCommandBuffers[i], 6, 1, 0, 0, 0);

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

VkShaderModule createShaderModule(const char *filename)
{
    FILE *fptr = fopen(filename, "rb");
    fseek(fptr, 0, SEEK_END);
    size_t fileSize = ftell(fptr);
    uint8_t *fileBytes = (uint8_t *)alloca(fileSize);
    fseek(fptr, 0, SEEK_SET);
    fread(fileBytes, 1, fileSize, fptr);
    fclose(fptr);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = (uint32_t *)fileBytes;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(s_device.vkDevice, &createInfo, NULL, &shaderModule) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create shader module for %s\n", filename);
        exit(1);
    }

    fprintf(stdout, "created shader module for %s\n", filename);

    return shaderModule;
}

VkBool32 getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags  properties, uint32_t* typeIndex)
{
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(s_device.vkPhysicalDevices[0], &deviceMemoryProperties);

    for (uint32_t i = 0; i < 32; i++)
    {
        if (typeBits & (1 << i))    // && m_vkPhysicalDeviceMemoryProperties[0].memoryTypes[i].propertyFlags & properties) == properties)
        {
            const VkMemoryType* type = &deviceMemoryProperties.memoryTypes[i];
            if ((type->propertyFlags & properties) == properties)
            {
                *typeIndex = i;
                return true;
            }
        }
//        typeBits >>= 1;
    }
    fprintf(stderr, "Could not find memory type to satisfy requirements\n");
    return false;
}

VkCommandBuffer beginSingleUseCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = s_device.vkCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(s_device.vkDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleUseCommandBuffer(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(s_device.vkDevice, &fenceCreateInfo, NULL, &fence) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create fence.\n");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(s_device.vkGraphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to submit to queue.\n");
    }

    if (vkWaitForFences(s_device.vkDevice, 1, &fence, VK_TRUE, kDefaultFenceTimeout) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to wait on fence.\n");
    }

    vkDestroyFence(s_device.vkDevice, fence, NULL);

    vkFreeCommandBuffers(s_device.vkDevice, s_device.vkCommandPool, 1, &commandBuffer);
}

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange* subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange = *subresourceRange;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ((oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED || oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;    // VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        fprintf(stderr, "unsupported layout transition!\n");
        exit(1);
    }
    
//    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void copyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height)
{
    VkImageSubresourceLayers subResource = {};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.baseArrayLayer = 0;
    subResource.mipLevel = 0;
    subResource.layerCount = 1;

    VkImageCopy region = {};
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = (VkOffset3D){ 0, 0, 0 };
    region.dstOffset = (VkOffset3D){ 0, 0, 0 };
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
