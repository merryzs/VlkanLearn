#include "Renderer.h"
#include "debug.h"
#include "application.h"
#include "vertex.h"
#include "UniformBufferObl.h"

#include <iostream>
#include <GLFW/glfw3.h>
#include <fstream>

#include <algorithm>
#include <assert.h>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>
#include <chrono>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#ifdef NDEBUG 
constexpr bool enableValidationLayers = false;
#else 
constexpr bool enableValidationLayers = true;
#endif









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
    
    vk::InstanceCreateInfo instanceCreateInfo
    {
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayer.size()),
        .ppEnabledLayerNames = requiredLayer.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };
    instance = vk::raii::Instance(context, instanceCreateInfo);
    
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createlogicalDevice();
    createSwapchain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSet();
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
    while ((width == 0 || height == 0) && !glfwWindowShouldClose(window))
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
    createDepthResources();
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
    
    swapChainImageViews.reserve(swapChainImages.size());
    for ( auto &image: swapChainImages )
    {
        swapChainImageViews.emplace_back(createImageView(image, swapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor));
    }
}

void Renderer::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{{.binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex},{.binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()}; 
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
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
    
    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False, .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack, .frontFace = vk::FrontFace::eCounterClockwise, .depthBiasEnable = vk::False, .lineWidth = 1.0f};    
    vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};
    
    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{ .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0};
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);
    
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain ={{ .stageCount = 2, .pStages = ShaderStage, .pVertexInputState = &vertexInputInfo,.pInputAssemblyState = &inputAssembly, .pViewportState = &viewportState, .pRasterizationState = &rasterizer, .pMultisampleState = &multisampling,.pColorBlendState = &colorBlending, .pDynamicState = &dynamicStateInfo, .layout = pipelineLayout, .renderPass = nullptr }, {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}





void Renderer::createCommandPool()
{
    vk::CommandPoolCreateInfo poolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueIndex };
    commandPool = vk::raii::CommandPool(device, poolCreateInfo);
}

void Renderer::createTextureImage()
{
    int texWidth , texHeight, textChannel;
    stbi_uc *pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &textChannel, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image");
    }
    
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();
    stbi_image_free(pixels);
    
    std::tie(textureImage, textureImageMemory) = createImage( texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    vk::raii::CommandBuffer commandBuffer = beginSinfleTimeCommands();
    transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(commandBuffer, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    endSingleTimeCommands(std::move(commandBuffer));
    
}

void Renderer::createTextureImageView()
{
    textureImageView = createImageView( *textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void Renderer::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    
    vk::SamplerCreateInfo samplerInfo{ .magFilter = vk::Filter::eLinear, .minFilter = vk::Filter::eLinear, .mipmapMode = vk::SamplerMipmapMode::eLinear, .addressModeU = vk::SamplerAddressMode::eRepeat, .addressModeV = vk::SamplerAddressMode::eRepeat, .addressModeW = vk::SamplerAddressMode::eRepeat, .mipLodBias = 0.0f, .anisotropyEnable = vk::True, .maxAnisotropy = properties.limits.maxSamplerAnisotropy, .compareEnable = vk::False, .compareOp = vk::CompareOp::eAlways};
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Renderer::createImage(uint32_t width, uint32_t height,vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::ImageCreateInfo imageCreateInfo{ .imageType = vk::ImageType::e2D, .format = format, .extent =  {width, height, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = vk::SampleCountFlagBits::e1, .tiling = tiling, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Image image = vk::raii::Image(device, imageCreateInfo);
    vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{ .allocationSize = memoryRequirements.size, .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    image.bindMemory(*imageMemory, 0);
    return {std::move(image), std::move(imageMemory)};
}

void Renderer::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier barrier{.oldLayout           = oldLayout,
                                       .newLayout           = newLayout,
                                       .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                       .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                       .image               = image,
                                       .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
}

void Renderer::copyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer,vk::raii::Image& image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region{ .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0, .imageSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 }, .imageOffset = {0, 0, 0 }, .imageExtent = {width,height, 1 }};commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
    vk::raii::Buffer buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{.allocationSize =  memoryRequirements.size, .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};

    

}

vk::raii::CommandBuffer Renderer::beginSinfleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return std::move(commandBuffer);
}

vk::raii::ImageView Renderer::createImageView(vk::Image const &image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D, .format = format, .subresourceRange = {.aspectMask =  vk::ImageAspectFlagBits::eColor,.baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1} };
    return vk::raii::ImageView(device, viewInfo);
}

vk::Format Renderer::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (const auto format : candidates)
    {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);
        if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
            ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)))
        {
            return format;
        }
    }
    throw std::runtime_error{"failed to find supported format"};
}

void Renderer::endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    graphicQueue.submit(submitInfo, nullptr);
    graphicQueue.waitIdle();
}

void Renderer::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();
    std::tie(depthImage, depthImageMemory) = createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}


void Renderer::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();
    
    std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    
    
}

void Renderer::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize,  vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();
    
    std::tie(indexBuffer, indexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Renderer::createUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObl);
        auto [buffer, bufferMemory] = createBuffer( bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        
        uniformeBuffer.emplace_back(std::move(buffer));
        uniformBufferMemory.emplace_back(std::move(bufferMemory));
        uniformBufferMapped.emplace_back(uniformBufferMemory.back().mapMemory(0, bufferSize));
    }
}

void Renderer::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
    vk::raii::CommandBuffer commandCopyByffer = std::move(device.allocateCommandBuffers(allocInfo).front());
    
    commandCopyByffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyByffer.copyBuffer(* srcBuffer,* dstBuffer, vk::BufferCopy( 0, 0, size));
    commandCopyByffer.end();
    
    graphicQueue.submit(vk::SubmitInfo{.commandBufferCount =  1, .pCommandBuffers = &*commandCopyByffer }, nullptr);
    graphicQueue.waitIdle();
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
        commandBuffer.setViewport(0, vk::Viewport(0.0f, static_cast<float>(swapChainExtent.height), static_cast<float>(swapChainExtent.width), -static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
        commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0}),
        commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value),
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,0, *descriptorSets[frameIndex], nullptr),
		commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()),1, 0, 0, 0 );
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

void Renderer::updtUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime-startTime).count();
    UniformBufferObl ubo;
    
    ubo.model = rotate(glm::mat4(1.f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj =  glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
    
    memcpy(uniformBufferMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize{ .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT };
    vk::DescriptorPoolCreateInfo poolInfo{ .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, .maxSets = MAX_FRAMES_IN_FLIGHT, .poolSizeCount = 1, .pPoolSizes = &poolSize};
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void Renderer::createDescriptorSet()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo        allocInfo{
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()};

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformeBuffer[i], .offset = 0, .range = sizeof(UniformBufferObl)};
        vk::DescriptorImageInfo  imageInfo{.sampler = textureSampler, .imageView = textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{{.dstSet          = descriptorSets[i],
                                                                 .dstBinding      = 0,
                                                                 .dstArrayElement = 0,
                                                                 .descriptorCount = 1,
                                                                 .descriptorType  = vk::DescriptorType::eUniformBuffer,
                                                                 .pBufferInfo     = &bufferInfo},
                                                                {.dstSet          = descriptorSets[i],
                                                                 .dstBinding      = 1,
                                                                 .dstArrayElement = 0,
                                                                 .descriptorCount = 1,
                                                                 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                                 .pImageInfo      = &imageInfo}}};
        device.updateDescriptorSets(descriptorWrites, {});
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
    updtUniformBuffer(frameIndex);
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

vk::Format Renderer::findDepthFormat()
{
    return findSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
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
    bool supportsGraphicsAndPresent = std::ranges::any_of(queueFamilies, [&physicalDevice, &surface = this->surface, &qfpIndex](auto const &qfp) { bool const suitable = (qfp.queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface); qfpIndex++; return suitable;});

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

