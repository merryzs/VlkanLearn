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
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicQueue = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    
    
    
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createlogicalDevice();
    void createSurface();
    
    
    std::vector<char const*> GetRequiredExtensions(); 
    std::vector<char const*> GetRequiredLayers(); 
    
    
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_report_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback, void* );
    bool CheckDeviceSuitability(vk::raii::PhysicalDevice const&physicalDevice);
};
