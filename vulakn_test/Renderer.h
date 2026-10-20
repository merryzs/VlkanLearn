#pragma once
#include "dependency.h"



class Renderer
{
public:
    void createInstance();
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
   
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createlogicalDevice();
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createGraphicsPipeline();
    
    std::vector<vk::Image>  swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
   
    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName};
    std::vector<char const*> GetRequiredExtensions(); 
    std::vector<char const*> GetRequiredLayers(); 
    std::vector<char const*> GetRequiredInstanceExtensions();
    static std::vector<char> readFile(const std::string &filename);
    
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
    vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
    
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void * );
    bool CheckDeviceSuitability(vk::raii::PhysicalDevice const&physicalDevice);
};
