#include "Renderer.h"
#include "debug.h"

#include <iostream>
#include <GLFW/glfw3.h>





void Renderer::createInstance()
{
   
   constexpr vk::ApplicationInfo app_info
    {
        .pApplicationName = "Vulkan Test",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Triangle",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };
    
    std::vector requiredExtensions = GetRequiredExtensions();
    auto extensionsProperties = context.enumerateInstanceExtensionProperties();
    
    uint32_t glfwExtensionsCount = 0;
    auto extension = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    
    for (uint32_t i = 0; i < requiredExtensions.size(); ++i)
    {
       auto predicate = [extension = extension[i]] (auto const& extensionProperty)
            {
                return strcmp(extensionProperty.extensionName, extension) == 0;
            };
        
        if (std::ranges::none_of(extensionsProperties, predicate))
            {
                throw std::runtime_error("Required GLFW extension not supported: " + std::string(requiredExtensions[i]));
            }
    }
    
    const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else 
    bool enableValidationLayers = true;
#endif
    
    std::vector<const char*> requiredLayers;
    
    if (debug::EnableDebug)
    {
        requiredLayers.assign_range(validationLayers);
    }
    
    
    auto LayerProperties = context.enumerateInstanceLayerProperties();
    
    auto  predicate_layer = [&LayerProperties](auto const& requiredLayers)
    {
        return std::ranges::none_of( LayerProperties, [requiredLayers]  (auto const& LayerProperty) 
        {
            return strcmp(LayerProperty.layerName, requiredLayers)==0;
        });
    };
    
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, predicate_layer);
    
    if (unsupportedLayerIt != requiredLayers.end())
    {
        throw std::runtime_error("Required layer not supported: " +std::string(*unsupportedLayerIt));
    }
    
    vk::InstanceCreateInfo instance_create_info
    {
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
        
     
    };
    
    instance = vk::raii::Instance(context, instance_create_info);
    
    setupDebugMessenger();
}

std::vector<char const*> Renderer::GetRequiredExtensions()
{
    uint32_t glfwExtensionsCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    
    std::vector<const char*> applicationExtensions = { vk::EXTDebugMarkerExtensionName};

    
    std::vector requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
    
    requiredExtensions.append_range(applicationExtensions);
    
    return requiredExtensions;
}


std::vector<const char*> Renderer::GetRequiredLayers()
{
    const std::vector ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    
    
    std::vector<const char*> requiredLayers = {};
    
    if (debug::EnableDebug)
        requiredLayers.append_range(ValidationLayers);
    
    return requiredLayers;
}

void Renderer::setupDebugMessenger()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    
    vk::DebugUtilsMessageTypeFlagsEXT typeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
    
    vk::DebugUtilsMessengerCreateInfoEXT createInfo
    {
        .messageSeverity = severityFlags,
        .messageType = typeFlags,
        .pfnUserCallback = &DebugReportCallback,
    };
    
    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
}


vk::Bool32 Renderer::DebugReportCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback, void*)
{
    std::cout << "Validation layer: [" << vk::to_string(type) << "] message : " << callback->pMessage << std::endl;
    
    return vk::False;
}


std::vector requiresDeviceExtensions = {vk::KHRSwapchainExtensionName};

void Renderer::pickPhysicalDevice()
{
    
    auto physicalDevices = vk::raii::PhysicalDevices(instance);
    
    if (physicalDevices.empty())
    {
        throw std::runtime_error("No physical devices (GPU) found");
    }
    
}

bool Renderer::CheckDeviceSuitability(vk::raii::PhysicalDevice const& physicalDevice)
{
    auto deviceProperties = physicalDevice.getProperties();
    auto deviceFeatures = physicalDevice.getFeatures();
    
    
    bool CanUseVulkan1_3 = deviceProperties.apiVersion >= vk::ApiVersion13;
    
    auto QueueFamiy = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of(QueueFamiy, [](auto const& qfp) {return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && deviceFeatures.geometryShader)
    {
        return true;
    }
    return false;
}

