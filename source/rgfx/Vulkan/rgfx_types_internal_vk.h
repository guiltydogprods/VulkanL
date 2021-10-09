//
//  rgfx_types_internal_vk.h
//  VulkanL
//
//  Created by Claire Rogers on 23/09/2021.
//

#ifndef rgfx_types_internal_vk_h
#define rgfx_types_internal_vk_h

//typedef struct rgfx_device { int32_t id; } rgfx_device;
typedef struct rgfx_gpu_mem_block_vk { int32_t id; } rgfx_gpu_mem_block_vk;

typedef struct rgfx_device_vk
{
    VkInstance                          vkInstance;
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
    VkImage								vkSwapChainImages[3];
    VkImageView							vkSwapChainImageViews[3];
    VkFramebuffer						vkSwapChainFramebuffers[3];
    
    VkCommandPool						vkCommandPool;
    
    VkRenderPass						vkRenderPass;

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

typedef struct rgfx_gpu_mem_block_info_vk
{
//    GPUMemoryBlock(VkDevice device, uint32_t size, uint32_t typeIndex);
//    GPUMemoryBlock() {};
//    ~GPUMemoryBlock();

//    GPUMemAllocInfo& allocate(ScopeStack& scope, uint32_t size, uint32_t alignment, uint32_t optimal);

    VkDevice vkDevice;
    VkDeviceMemory memory;
    uint32_t size;
    uint32_t offset;
    uint32_t typeIndex;
    uint32_t optimal;
}rgfx_gpu_mem_block_info_vk;

typedef struct rgfx_gpu_alloc_info_vk
{
    rgfx_gpu_mem_block_vk memoryBlock;
    uint32_t size;
    uint32_t offset;
}rgfx_gpu_alloc_info_vk;


#endif /* rgfx_types_internal_vk_h */
