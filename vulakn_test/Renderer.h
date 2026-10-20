#pragma once
#include "dependency.h"



class Renderer
{
public:
    void createInstance();
    void drawFrame();

    GLFWwindow* window = nullptr;
    
private : 
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicQueue = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffer commandBuffer = nullptr;
    
    vk::raii::Semaphore presentCompletSemaphore = nullptr;
    vk::raii::Semaphore renderCompleteSemaphore = nullptr;
    vk::raii::Fence drawFence = nullptr;
   
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    
    uint32_t queueIndex = ~0;
    
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createlogicalDevice();
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex);
    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask);
    void createSyncObjects();
    
    
    std::vector<vk::Image>  swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
   
    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName};
   static std::vector<char const*> GetRequiredExtensions(); 
    static std::vector<char const*> GetRequiredLayers(); 
    std::vector<char const*> GetRequiredInstanceExtensions();
    static std::vector<char> readFile(const std::string &filename);
    
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
    vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
    
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void * );
    bool CheckDeviceSuitability(vk::raii::PhysicalDevice const&physicalDevice);
    
    
    static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);


};
