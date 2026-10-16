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
    createSurface();
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

vk::Bool32 Renderer::debug_report_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* data, void*)
{
    std::cout << "Validation layer: [" << vk::to_string(type) << "] message : " << data->pMessage << std::endl;
    
    return vk::False;
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
    
    vk::DebugUtilsMessageTypeFlagsEXT typeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
    
    vk::DebugUtilsMessengerCreateInfoEXT createInfo
    {
        .messageSeverity = severityFlags,
        .messageType = typeFlags,
        .pfnUserCallback = &debug_report_callback
    };
    
    try
    {
        debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
    }
    catch (std::exception e)
    {
        std::cout << e.what() << std::endl;
    }

    
}






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
    bool supportsAllRequiredExtentions = std::ranges::all_of(GetRequiredExtensions(), [&availableDevicesExtensions](auto const& requiredDeviceExtensions)
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
    
    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
        {
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error( "Could not find a queue for graphics and present -> terminating" );
    }
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = { {},{.shaderDrawParameters = true},{.dynamicRendering = true},{.extendedDynamicState = true}};
    
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo queueInfo  { .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
    vk::DeviceCreateInfo deviceCreateInfo { .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(), .queueCreateInfoCount = 1, .pQueueCreateInfos = &queueInfo, .enabledExtensionCount = static_cast<uint32_t>( requiredDeviceExtension().size()), .ppEnabledExtensionNames = requiredDeviceExtension().data()};

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicQueue = vk::raii::Queue(device, queueIndex, 0);
}




void Renderer::createSurface()
{
    vk::Win32SurfaceCreateInfoKHR createInfoSurface = {.hinstance = GetModuleHandle(nullptr), .hwnd = glfwGetWin32Window(window)};
    surface = instance.createWin32SurfaceKHR(createInfoSurface);
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR( instance, _surface );
}