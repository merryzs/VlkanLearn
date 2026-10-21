#include "Renderer.h"
#include "debug.h"
#include "application.h"
#include "vertex.h"

#include <iostream>
#include <GLFW/glfw3.h>
#include <fstream>

#include <algorithm>
#include <assert.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>



#ifdef NDEBUG 
constexpr bool enableValidationLayers = false;
#else 
constexpr bool enableValidationLayers = true;
#endif





const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f,1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f,1.0f,0.0f}},
    {{-0.5f, 0.5f}, {0.0f,0.0f,1.0f}}};



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
        .ppEnabledExtensionNames = requiredExtensions.data()
    };
    instance = vk::raii::Instance(context, instance_create_info);
    
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createlogicalDevice();
    createSwapchain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}



void Renderer::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app =reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Renderer::setupDebugMessenger()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    
    vk::DebugUtilsMessageTypeFlagsEXT typeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
    
    vk::DebugUtilsMessengerCreateInfoEXT createInfo
    {
        .messageSeverity = severityFlags,
        .messageType = typeFlags,
        .pfnUserCallback = &debugCallback
    };
        debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
  
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

void Renderer::createlogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex)
    {
        if ( queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics && 
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
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
    vk::DeviceCreateInfo deviceCreateInfo { .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(), .queueCreateInfoCount = 1, .pQueueCreateInfos = &queueInfo, .enabledExtensionCount = static_cast<uint32_t>( requiredDeviceExtension.size()), .ppEnabledExtensionNames = requiredDeviceExtension.data()};

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicQueue = vk::raii::Queue(device, queueIndex, 0);
}



void Renderer::createSurface()
{
    VkSurfaceKHR _surface;
  
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR( instance, _surface );
}

void Renderer::cleanupSwapChain()
{
    swapChainImageViews.clear();
    swapChain = nullptr;
}

void Renderer::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while ((width == 0) || (height == 0) && !glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    if (glfwWindowShouldClose(window))
    {
        return;
    }
    
    device.waitIdle();
    cleanupSwapChain();
    
    createSwapchain();
    createImageViews();
}

void Renderer::createSwapchain()
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapChainExtent = chooseSwapExtent(surfaceCapabilities); 
    uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);
    
    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
    
    std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);
    
    vk::SwapchainCreateInfoKHR swapchainCreateInfo = { .surface = *surface, .minImageCount = minImageCount, .imageFormat = swapChainSurfaceFormat.format, .imageColorSpace = swapChainSurfaceFormat.colorSpace, .imageExtent = swapChainExtent, .imageArrayLayers = 1, .imageUsage = vk::ImageUsageFlagBits::eColorAttachment, .imageSharingMode = vk::SharingMode::eExclusive, .preTransform = surfaceCapabilities.currentTransform, .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque, .presentMode = presentMode, .clipped = true};
    
    swapChain = vk::raii::SwapchainKHR(device, swapchainCreateInfo);
    swapChainImages = swapChain.getImages();
}



void Renderer::createImageViews()
{
    assert(swapChainImageViews.empty());
    
    vk::ImageViewCreateInfo imageViewCreateInfo = { .viewType = vk::ImageViewType::e2D, .format = swapChainSurfaceFormat.format, .subresourceRange = {vk::ImageAspectFlagBits::eColor,0,1,0,1}};

    for (auto &image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

void Renderer::createGraphicsPipeline()
{
    vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"));
    
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    vk::PipelineShaderStageCreateInfo ShaderStage[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{.vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()), .pVertexAttributeDescriptions = attributeDescriptions.data() };
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
    
    vk::PipelineRasterizationStateCreateInfo rasterizer{ .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False, .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack, .frontFace = vk::FrontFace::eClockwise, .depthBiasEnable = vk::False, .lineWidth = 1.f};
    
    vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};
    
    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ .setLayoutCount = 0, .pushConstantRangeCount = 0};
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);
    
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain ={{ .stageCount = 2, .pStages = ShaderStage, .pVertexInputState = &vertexInputInfo,.pInputAssemblyState = &inputAssembly, .pViewportState = &viewportState, .pRasterizationState = &rasterizer, .pMultisampleState = &multisampling,.pColorBlendState = &colorBlending, .pDynamicState = &dynamicStateInfo, .layout = pipelineLayout, .renderPass = nullptr }, {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}





void Renderer::createCommandPool()
{
    vk::CommandPoolCreateInfo poolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueIndex };
    commandPool = vk::raii::CommandPool(device, poolCreateInfo);
}

void Renderer::createVertexBuffer()
{
    vk::BufferCreateInfo bufferInfo{.size = sizeof(vertices[0]) * vertices.size(), .usage = vk::BufferUsageFlagBits::eVertexBuffer, .sharingMode =  vk::SharingMode::eExclusive};
    vertexBuffer = vk::raii::Buffer(device, bufferInfo);
    
    vk::MemoryRequirements memoryRequirements = vertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{.allocationSize =  memoryRequirements.size, .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)};
    
    vertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    vertexBuffer.bindMemory(*vertexBufferMemory, 0);

    void* data = vertexBufferMemory.mapMemory(0, bufferInfo.size);
    memcpy(data, vertices.data(), bufferInfo.size);
    vertexBufferMemory.unmapMemory();
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1<<i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find a suitable memory type!");
}


void Renderer::createCommandBuffers()
{
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    auto &commandBuffer = commandBuffers[frameIndex];
    commandBuffer.begin({});
    
        transition_image_layout( imageIndex, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
		vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attachmentInfo = { .imageView   = swapChainImageViews[imageIndex], .imageLayout = vk::ImageLayout::eColorAttachmentOptimal, .loadOp      = vk::AttachmentLoadOp::eClear, .storeOp     = vk::AttachmentStoreOp::eStore, .clearValue  = clearColor};
		vk::RenderingInfo renderingInfo = {
		    .renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &attachmentInfo};

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
		commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
        commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0}),
		commandBuffer.draw(3, 1, 0, 0);
		commandBuffer.endRendering();

		transition_image_layout( imageIndex, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);
		commandBuffer.end();
}

void Renderer::transition_image_layout( uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
{
    vk::ImageMemoryBarrier2 barrier = { .srcStageMask = src_stage_mask, .srcAccessMask = src_access_mask, .dstStageMask = dst_stage_mask, .dstAccessMask = dst_access_mask, .oldLayout = old_layout, .newLayout = new_layout, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = swapChainImages[imageIndex], .subresourceRange ={ .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel   = 0, .levelCount     = 1, .baseArrayLayer = 0, .layerCount     = 1}};
    vk::DependencyInfo dependency_info = { .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
    commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
}

void Renderer::createSyncObjects()
{
    assert(presentCompletSemaphores.empty() && renderCompleteSemaphores.empty() && drawFence.empty());
    for (size_t i = 0 ; i < swapChainImages.size() ; i++)
    {
        renderCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }
    for (size_t i = 0 ; i < MAX_FRAMES_IN_FLIGHT ; i++)
    {
        presentCompletSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        drawFence.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
    
}

void Renderer::drawFrame()
{
    auto fenceResult = device.waitForFences(*drawFence[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }
    device.resetFences(*drawFence[frameIndex]);
    
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompletSemaphores[frameIndex], nullptr);
    
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);
    
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*presentCompletSemaphores[frameIndex], .pWaitDstStageMask = &waitDestinationStageMask, .commandBufferCount = 1, .pCommandBuffers = &*commandBuffers[frameIndex], .signalSemaphoreCount = 1, .pSignalSemaphores = &*renderCompleteSemaphores[imageIndex]};
    graphicQueue.submit(submitInfo, *drawFence[frameIndex]);
    
    const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*renderCompleteSemaphores[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex };
    result = graphicQueue.presentKHR(presentInfoKHR);
    
    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    }
    else
    {
        assert(result == vk::Result::eSuccess);
    }
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    
}

std::vector<char const*> Renderer::GetRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector required_extensions ={ vk::EXTDebugUtilsExtensionName };
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




std::vector<char const*> Renderer::GetRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return extensions;
}

std::vector<char> Renderer::readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
   
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}

vk::Extent2D Renderer::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize( window, &width, &height );
    
    return { 
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width), 
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}


[[nodiscard]] vk::raii::ShaderModule Renderer::createShaderModule(const std::vector<char> &code) const
{
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t *>(code.data())};
    vk::raii::ShaderModule     shaderModule{ device, createInfo};

    return shaderModule;
}

vk::Bool32 Renderer::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* data, void*)
{
    std::cout << "Validation layer: [" << vk::to_string(type) << "] message : " << data->pMessage << '\n';
    
    return vk::False;
}
    
bool Renderer::CheckDeviceSuitability(vk::raii::PhysicalDevice const& physicalDevice)
{
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
    
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    uint32_t qfpIndex = 0;
    bool supportsGraphicsAndPresent = 	    std::ranges::any_of(queueFamilies,
                                [&physicalDevice, &surface = this->surface, &qfpIndex](auto const &qfp) {
                                    bool const suitable = (qfp.queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface);
                                    qfpIndex++;
                                    return suitable;
                                });

    auto availableDevicesExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtentions = std::ranges::all_of(requiredDeviceExtension, [&availableDevicesExtensions](auto const& requiredDeviceExtensions)
    {
        return std::ranges::any_of(availableDevicesExtensions, [requiredDeviceExtensions](auto const& availableDevicesExtensions) 
                                   { return strcmp( availableDevicesExtensions.extensionName, requiredDeviceExtensions) == 0; });
    } );
    
    auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> ();
    
    bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters && features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
    
    return supportsVulkan1_3 && supportsGraphicsAndPresent && supportsAllRequiredExtentions && supportsRequiredFeatures;
}

uint32_t Renderer::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ( (0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format) {return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}



vk::PresentModeKHR Renderer::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
    assert(std::ranges::any_of(availablePresentModes, [] (auto presentMode) {return presentMode == vk::PresentModeKHR::eFifo;}));
    return std::ranges::any_of(availablePresentModes, [] (const vk::PresentModeKHR value) {return vk::PresentModeKHR::eMailbox == value; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}
