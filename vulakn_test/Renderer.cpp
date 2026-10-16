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
    
    
    for (uint32_t i = 0; i < requiredExtensions.size(); ++i)
    {
        auto compareExtensions = [extention = requiredExtensions[i]](auto const& extensionProperty)
        {
            return strcmp(extensionProperty.extensionName, extention)==0;
        };
        if (std::ranges::none_of(extensionsProperties, compareExtensions))
        {
            throw std::runtime_error("Required GLFW extensions not supported: " +std::string(requiredExtensions[i]));
        }
    }
    
    std::vector requiredLayers = GetRequiredLayers();
    auto layerProperties = context.enumerateInstanceLayerProperties();
    
    const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};
    
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else 
    bool enableValidationLayers = true;
#endif
    
    std::vector<const char*> requiredLayer;
    
    if (debug::EnableDebug)
    {
        requiredLayers.assign_range(validationLayers);
    }
    
    
    auto LayerProperties = context.enumerateInstanceLayerProperties();
    
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](auto const& requiredLayer)
    {
        return std::ranges::none_of(layerProperties, [requiredLayer] (auto const& LayerProperty)
        {
            return strcmp(LayerProperty.layerName, requiredLayer)==0;
        });
    });
    if (unsupportedLayerIt != requiredLayers.end())
    {
        throw std::runtime_error("Unsupported layer: " + std::string(*unsupportedLayerIt));
    }
    
    vk::InstanceCreateInfo instance_create_info
    {
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
        
     
    };

   try
   {
       instance = vk::raii::Instance(context, instance_create_info);
   }
   catch (std::exception e)
   {
       std::cout << e.what() << std::endl;
   }
    
    setupDebugMessenger();
    pickPhysicalDevice();
    createlogicalDevice();
}

std::vector<char const*> Renderer::GetRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char*> required_extensions ={};
    std::vector extensions(glfwExtensions, glfwExtensions + glfw_extension_count);
    
    extensions.append_range(required_extensions);
    
    return extensions;
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
    std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    
    auto const DevIt = std::ranges::find_if( physicalDevices, [&](auto const& physicalDevice) {return CheckDeviceSuitability( physicalDevice ); } );
    
    if (DevIt == physicalDevices.end())
    {
        throw std::runtime_error( "failed to find a suitable physical device" );
    }
    physicalDevice = *DevIt;
}


bool Renderer::CheckDeviceSuitability(vk::raii::PhysicalDevice const& physicalDevice)
{
    bool CanUseVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
    
    auto QueueFamilies = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of(QueueFamilies, [](auto const& qfp) {return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    auto availableDevicesExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtentions = std::ranges::all_of(requiresDeviceExtensions, [&availableDevicesExtensions](auto const& requiredDeviceExtensions)
    {
        return std::ranges::any_of(availableDevicesExtensions, [requiredDeviceExtensions](auto const& availableDevicesExtensions) 
        { return strcmp( availableDevicesExtensions.extensionName, requiredDeviceExtensions) == 0; });
    } );
    
    auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> ();
    
    bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters && features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
    
    return CanUseVulkan1_3 && supportsGraphics && supportsAllRequiredExtentions && supportsRequiredFeatures;
}


void Renderer::createlogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
   
    auto graphicsQueueFamilyProperty = std::ranges::find_if( queueFamilyProperties, [] (auto const &qfp) { return ( qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlagBits>(0); } );
    auto graphicIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty)) ;
    
    
    vk::DeviceQueueCreateInfo queueInfo  = 
    {
        .queueFamilyIndex = graphicIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
}

