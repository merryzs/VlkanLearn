#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "vulkan/vulkan_raii.hpp"

class Renderer
{
public:
    void createInstance();
    
private : 
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    
    
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createlogicalDevice();
    std::vector<char const*> GetRequiredExtensions(); 
    std::vector<char const*> GetRequiredLayers(); 
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugReportCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity , vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback, void*);

    bool CheckDeviceSuitability(vk::raii::PhysicalDevice const&physicalDevice);
};
