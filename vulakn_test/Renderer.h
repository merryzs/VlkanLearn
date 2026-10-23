#pragma once
#include "dependency.h"


class Renderer
{
public:
    void createInstance();
    void drawFrame();

    GLFWwindow* window = nullptr;
    int width = 800;
    int height = 600;
    
private : 
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicQueue = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;

    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;
    vk::raii::Buffer indexBuffer = nullptr;
    vk::raii::DeviceMemory indexBufferMemory = nullptr;
    
    vk::raii::Image        textureImage       = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    
    std::vector<vk::raii::Semaphore> presentCompletSemaphores;
    std::vector<vk::raii::Semaphore> renderCompleteSemaphores;
    std::vector<vk::raii::Fence> drawFence;
    
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets ;
    
    
    std::vector<vk::raii::Buffer> uniformeBuffer;
    std::vector<vk::raii::DeviceMemory> uniformBufferMemory;
    std::vector<void *> uniformBufferMapped;
   
    std::vector<const char*> requiredLayer;

    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    
    
    uint32_t queueIndex = ~0;
    uint32_t frameIndex = 0;
    static constexpr  int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);


    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createlogicalDevice();
    void createSurface();
    void cleanupSwapChain();
    void recreateSwapChain();
    void createSwapchain();
    void createImageViews();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createTextureImage();
    void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height);
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size);
    void createCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex);
    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask);
    void createSyncObjects();
    void updtUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSet();
    void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer);
    std::vector<char const*> GetRequiredInstanceExtensions();
    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName};
    
    static std::vector<char const*> GetRequiredExtensions(); 
    static std::vector<char const*> GetRequiredLayers(); 
    static std::vector<char> readFile(const std::string &filename);
    static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void * );
    
    bool CheckDeviceSuitability(vk::raii::PhysicalDevice const&physicalDevice);
    bool framebufferResized = false;
    
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
    vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
    
 
    std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage( uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);


    vk::raii::CommandBuffer beginSinfleTimeCommands();
};