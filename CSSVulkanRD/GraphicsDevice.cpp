#include "GraphicsDevice.h"
#include "GPUBuffer.h"
#include "Shader.h"
#include "GPUMemoryManager.h"
#include "DeviceContext.h"
#include "PipelineState.h"

GraphicsDevice::GraphicsDevice(GLFWwindow* pAppWindow)
{
    pApplicationWindow = pAppWindow;
    pipelineDirty = false;
}

GraphicsDevice::~GraphicsDevice()
{
    cleanup();
}

void GraphicsDevice::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    CreateSurface();
    PickPhysicalGPU();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    //createGraphicsPipeline();
    createFramebuffers();
    createCommandPools();
    createDescriptorPool();

    GetGPUProperties();
    initializeMainMemoryManager();

    auto qfi = FindQueueFamilies(physicalGPU);

    transferContext->SetQueue(transferQueues[0]);
    immediateContext->SetQueue(primaryGraphicsQueue);
}

void GraphicsDevice::cleanup()
{
    WaitForGPUIdle();

    cleanupSwapchain();

    vkDestroyDescriptorSetLayout(GPU, descriptorSetLayout, nullptr);

    vkDestroyDevice(GPU, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(pApplicationWindow);
    glfwTerminate();
}

void GraphicsDevice::cleanupSwapchain()
{
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(GPU, swapChainFramebuffers[i], nullptr);
    }

    vkDestroyFence(GPU, acquireImageFence, nullptr);

    pShader->Destroy();

    immediateContext->Destroy();
    transferContext->Destroy();

    for (int i = 0; i < inflightFrames.size(); i++)
    {
        vkDestroySemaphore(GPU, inflightFrames[i]->imageAvailable, nullptr);
        vkDestroySemaphore(GPU, inflightFrames[i]->renderFinished, nullptr);

        delete inflightFrames[i];
        inflightFrames.erase(inflightFrames.begin() + i);
    }

    vkDestroyDescriptorPool(GPU, descriptorPool, nullptr);

    vkDestroyPipeline(GPU, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(GPU, pipelineLayout, nullptr);
    vkDestroyRenderPass(GPU, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(GPU, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(GPU, swapChain, nullptr);
}

void GraphicsDevice::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "CatastrophicEngineVK_R&D";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "CatastrophicEngineVK";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    VULKAN_CALL_ERROR(vkCreateInstance(&createInfo, nullptr, &instance), "failed to create instance!");
}

void GraphicsDevice::createLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalGPU);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    std::vector<std::pair<uint32_t,uint32_t>> uniqueQueueFamilies = { 
        std::pair<uint32_t,uint32_t>(indices.graphicsFamily.value(),indices.graphicsQueueCount),
        std::pair<uint32_t,uint32_t>(indices.transferFamily.value(),indices.transferQueueCount),
        std::pair<uint32_t,uint32_t>(indices.computeFamily.value(),indices.computeQueueCount) 
    };

    float queuePriority = 1.0f;
    for(int i = 0; i < uniqueQueueFamilies.size(); i++)
    {
        auto queueFamily = uniqueQueueFamilies[i];

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

        queueCreateInfo.queueFamilyIndex = queueFamily.first;

        if (i != 0) //all other queues other than index 0 (graphics) will have as many queues as the gpu supports
        {
            queueCreateInfo.queueCount = queueFamily.second;
            float* pQueuePriorities = new float[queueCreateInfo.queueCount];

            for (int i = 0; i < queueCreateInfo.queueCount; i++) pQueuePriorities[i] = 1.0f;

            queueCreateInfo.pQueuePriorities = pQueuePriorities;
        }
        else
        {
            queueCreateInfo.queueCount = 1; //only 1 graphics queue for performance reasons
            queueCreateInfo.pQueuePriorities = &queuePriority;
        }

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures DeviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &DeviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    VULKAN_CALL_ERROR(vkCreateDevice(physicalGPU, &createInfo, nullptr, &GPU), "failed to create logical device!");

    vkGetDeviceQueue(GPU, indices.graphicsFamily.value(), 0, &primaryGraphicsQueue);
    vkGetDeviceQueue(GPU, indices.presentFamily.value(), 0, &presentQueue);

    //get transfer queues

    for (int i = 0; i < indices.transferQueueCount; i++)
    {
        VkQueue transferQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(GPU, indices.transferFamily.value(), i, &transferQueue);
        transferQueues.push_back(transferQueue);
    }

    //get compute queues

    for (int i = 0; i < indices.computeQueueCount; i++)
    {
        VkQueue computeQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(GPU, indices.computeFamily.value(), i, &computeQueue);
        computeQueues.push_back(computeQueue);
    }
}

void GraphicsDevice::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalGPU);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    QueueFamilyIndices indices = FindQueueFamilies(physicalGPU);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VULKAN_CALL_ERROR(vkCreateSwapchainKHR(GPU, &createInfo, nullptr, &swapChain), "failed to create swapchain");

    vkGetSwapchainImagesKHR(GPU, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(GPU, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void GraphicsDevice::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image    = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format   = swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VULKAN_CALL_ERROR(vkCreateImageView(GPU, &createInfo, nullptr, &swapChainImageViews[i]), "failed to create image views");
    }
}

void GraphicsDevice::createGraphicsPipeline() //DEPRECATED DEMO CODE -- to be refactored
{
    pShader = new Shader(GPU);
    pShader->LoadShader("shaders\\vs.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
    pShader->LoadShader("shaders\\ps.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.pName = pShader->GetShader(VK_SHADER_STAGE_VERTEX_BIT)->entrypoint;
    vertexStageInfo.module = pShader->GetShader(VK_SHADER_STAGE_VERTEX_BIT)->module;

    VkPipelineShaderStageCreateInfo pixelStageInfo{};
    pixelStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pixelStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pixelStageInfo.pName = pShader->GetShader(VK_SHADER_STAGE_FRAGMENT_BIT)->entrypoint;
    pixelStageInfo.module = pShader->GetShader(VK_SHADER_STAGE_FRAGMENT_BIT)->module;

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, pixelStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

    auto bindingDesc = VertexPositionColor::GetBindingDescription();
    auto attrDesc = VertexPositionColor::GetVertexAttributeDescriptions();

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    //---
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    //VkPushConstantRange pcr_vertex_transforms{}; //todo: implement push constant management system
    //pcr_vertex_transforms.offset = 0;
    //pcr_vertex_transforms.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    //pcr_vertex_transforms.size = sizeof(glm::mat4) * 3;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VULKAN_CALL_ERROR(vkCreatePipelineLayout(GPU, &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional

    VULKAN_CALL_ERROR(vkCreateGraphicsPipelines(GPU, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline), "Failed to create graphics pipeline!");
}

void GraphicsDevice::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};

    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VULKAN_CALL_ERROR(vkCreateRenderPass(GPU, &renderPassInfo, nullptr, &renderPass), "failed to create render pass");
}

void GraphicsDevice::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VULKAN_CALL_ERROR(vkCreateFramebuffer(GPU, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), "failed to create framebuffer");
    }
}

void GraphicsDevice::createCommandPools()
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalGPU);

    transferContext = std::make_shared<DeviceContext>();
    immediateContext = std::make_shared<DeviceContext>();

    immediateContext->Create(GPU, queueFamilyIndices.graphicsFamily.value());
    transferContext->Create(GPU, queueFamilyIndices.transferFamily.value());

    ImmediateContext = immediateContext;
    TransferContext = transferContext;
}

void GraphicsDevice::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(pApplicationWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(pApplicationWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(GPU);

    cleanupSwapchain();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();

    createCommandPools(); //7-8-2020 to fix window min/maximization bug
    createDescriptorPool();
    //todo: fire some kind of event so that the "user" code can re-create any gpu resources.

    PrepareFrame();
}

void GraphicsDevice::DrawFrame()
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { pActiveFrame->imageAvailable };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pActiveFrame->cmdBuffer->handle;
    VkSemaphore signalSemaphores[] = { pActiveFrame->renderFinished };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(GPU, 1, &pActiveFrame->cmdBuffer->fence);
    vkQueueSubmit(primaryGraphicsQueue, 1, &submitInfo, pActiveFrame->cmdBuffer->fence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex; //possible issue 6-16

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void GraphicsDevice::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VULKAN_CALL_ERROR(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "failed to set up debug messenger!");
}

std::vector<const char*> GraphicsDevice::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void GraphicsDevice::SetPushConstants(VkShaderStageFlags stage, size_t size, const void* pConstantData)
{
    vkCmdPushConstants(GetCurrentFrame()->cmdBuffer->handle, pipelineLayout, stage, 0, size, pConstantData);
}

InflightFrame* GraphicsDevice::GetCurrentFrame()
{
    return pActiveFrame;
}

bool GraphicsDevice::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void GraphicsDevice::CreateSurface() //possible weird shit
{
    VULKAN_CALL_ERROR(glfwCreateWindowSurface(instance, pApplicationWindow, nullptr, &surface), "error creating window surface");
}

void GraphicsDevice::createDescriptorPool() //deprecated
{
    VkDescriptorPoolSize cbPoolSize{};
    cbPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cbPoolSize.descriptorCount = 16;

    VkDescriptorPoolSize samplerPoolSize{};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = 4;

    std::array<VkDescriptorPoolSize, 2> poolSizes = { cbPoolSize, samplerPoolSize };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    VULKAN_CALL_ERROR(vkCreateDescriptorPool(GPU, &poolInfo, nullptr, &descriptorPool), "failed to create descriptor pool");
}

void GraphicsDevice::createDescriptorSetLayout() //deprecated
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VULKAN_CALL_ERROR(vkCreateDescriptorSetLayout(GPU, &layoutInfo, nullptr, &descriptorSetLayout), "Error creating descriptor set layout");
}

void GraphicsDevice::createDescriptorSets() //deprecated
{
    //TODO: go through registered descriptors and
    //TODO: eventially review nvidia presentation and move descriptor pool into device context

    uint32_t numDescriptorSets = registeredDescriptors.size() * swapChainImages.size();

    std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChainImages.size());

    VULKAN_CALL(vkAllocateDescriptorSets(GPU, &allocInfo, descriptorSets.data()));

    //TODO: interface portion after this with "user" code
}

void GraphicsDevice::RegisterShaderDescriptor(ShaderDescriptor* pDescriptor) //deprecated
{
    registeredDescriptors.push_back(pDescriptor);
    //possibly do more here
}

void GraphicsDevice::SetPipelineState(PipelineState* pState)
{
    pPipelineState = pState;
}

VkDescriptorSet GraphicsDevice::GetPipelineDescriptorSet(uint32_t index)
{
    return pPipelineState->GetDescriptorSet(index);
}

uint32_t GraphicsDevice::GetSwapchainFramebufferCount() const
{
    return swapChainFramebuffers.size();
}

VkExtent2D GraphicsDevice::GetSwapchainExtent() const
{
    return swapChainExtent;
}

VkRenderPass GraphicsDevice::GetRenderPass() const
{
    return renderPass;
}

VkDescriptorPool GraphicsDevice::GetDescriptorPool() const
{
    return descriptorPool;
}

PipelineState* GraphicsDevice::GetPipelineState() const
{
    return pPipelineState;
}



VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDevice::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << "\n" << std::endl;

    return VK_FALSE;
}

void GraphicsDevice::InitializeVulkan()
{
    initVulkan();
}

void GraphicsDevice::ShutdownVulkan()
{
    cleanup();
}

VkDevice GraphicsDevice::GetGPU() const
{
    return GPU;
}

VkPhysicalDevice GraphicsDevice::GetPhysicalDevice() const
{
    return physicalGPU;
}

VkPhysicalDeviceProperties GraphicsDevice::GetDeviceProperties() const
{
    return gpuProperties;
}

void GraphicsDevice::ResizeFramebuffer()
{
    framebufferResized = true;
}

int GraphicsDevice::PrepareFrame()
{
    pActiveFrame = GetAvailableFrame();
    VkResult res = vkAcquireNextImageKHR(GPU, swapChain, UINT64_MAX, pActiveFrame->imageAvailable, VK_NULL_HANDLE, &imageIndex);
    pActiveFrame->frameIndex = imageIndex;

    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
    }
    else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image!");

    return imageIndex;
}

std::shared_ptr<GPUMemoryManager> GraphicsDevice::GetMainGPUMemoryAllocator() const
{
    return memoryManager;
}

void GraphicsDevice::PrimaryGraphicsQueueSubmit(VkSubmitInfo submitInfo, bool block)
{
    vkQueueSubmit(primaryGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (block) vkQueueWaitIdle(primaryGraphicsQueue);
}

void GraphicsDevice::PrimaryTransferQueueSubmit(uint32_t transferQueueIndex, VkSubmitInfo submitInfo, bool block)
{
    VULKAN_CALL_ERROR(vkQueueSubmit(transferQueues[transferQueueIndex], 1, &submitInfo, VK_NULL_HANDLE), "failed to submit transfer queue");
}

VkQueue GraphicsDevice::GetTransferQueue(uint32_t index)
{
    return transferQueues[index];
}

VkQueue GraphicsDevice::GetComputeQueue(uint32_t index)
{
    return computeQueues[index];
}

std::shared_ptr<DeviceContext> GraphicsDevice::GetTransferContext() const
{
    return transferContext;
}

std::shared_ptr<DeviceContext> GraphicsDevice::CreateDeviceContext(VkQueueFlagBits queueType, bool transient)
{
    auto deviceContext = std::make_shared<DeviceContext>();
    auto qfi = FindQueueFamilies(physicalGPU);
    uint32_t queueFamily;

    switch (queueType)
    {
        case VK_QUEUE_GRAPHICS_BIT:
        {
            queueFamily = qfi.graphicsFamily.value();
            break;
        }
        case VK_QUEUE_TRANSFER_BIT:
        {
            queueFamily = qfi.transferFamily.value();
            break;
        }
        case VK_QUEUE_COMPUTE_BIT:
        {
            queueFamily = qfi.computeFamily.value();
            break;
        }
    }

    deviceContext->Create(GPU, queueFamily, transient);
    return deviceContext;
}

void GraphicsDevice::BeginRenderPass()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VULKAN_CALL(vkBeginCommandBuffer(pActiveFrame->cmdBuffer->handle, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { 0.100f, 0.149f, 0.255f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(pActiveFrame->cmdBuffer->handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(pActiveFrame->cmdBuffer->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
}

void GraphicsDevice::EndRenderPass()
{
    vkCmdEndRenderPass(pActiveFrame->cmdBuffer->handle);
    VULKAN_CALL(vkEndCommandBuffer(pActiveFrame->cmdBuffer->handle));
}

void GraphicsDevice::WaitForGPUIdle()
{
    vkDeviceWaitIdle(GPU);
}

QueueFamilyIndices GraphicsDevice::FindQueueFamilies(VkPhysicalDevice physicalGPU)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalGPU, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalGPU, &queueFamilyCount, queueFamilies.data());

    for(int i = 0; i < queueFamilyCount; ++i)
    {
        auto queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            indices.graphicsQueueCount = queueFamilies[i].queueCount;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalGPU, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }
        }

        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) //dedicated transfer queue family found
        {
            indices.transferFamily = i;
            indices.transferQueueCount = queueFamilies[i].queueCount;
        }

        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) //dedicated compute queue family found
        {
            indices.computeFamily = i;
            indices.computeQueueCount = queueFamilies[i].queueCount;
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool GraphicsDevice::IsDeviceSuitable(VkPhysicalDevice physicalGPU)
{
    QueueFamilyIndices indices = FindQueueFamilies(physicalGPU);

    bool extensionsSupported = CheckDeviceExtensionSupport(physicalGPU);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalGPU);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool GraphicsDevice::CheckDeviceExtensionSupport(VkPhysicalDevice physicalGPU)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalGPU, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalGPU, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void GraphicsDevice::PickPhysicalGPU()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physicalGPU = device;
            break;
        }
    }

    if (physicalGPU == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

SwapChainSupportDetails GraphicsDevice::QuerySwapChainSupport(VkPhysicalDevice physicalGPU)
{
    SwapChainSupportDetails swapDetails;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalGPU, surface, &swapDetails.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalGPU, surface, &formatCount, swapDetails.formats.data());

    if (formatCount != 0) {
        swapDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalGPU, surface, &formatCount, swapDetails.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalGPU, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        swapDetails.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalGPU, surface, &presentModeCount, swapDetails.presentModes.data());
    }

    return swapDetails;
}

VkSurfaceFormatKHR GraphicsDevice::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
}

VkExtent2D GraphicsDevice::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(pApplicationWindow, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

InflightFrame* GraphicsDevice::GetAvailableFrame()
{
    if (inflightFrames.size() == 0)
    {
        InflightFrame* NewFrame = CreateInflightFrame();
        inflightFrames.push_back(NewFrame);
        return NewFrame;
    }
    else
    {
        for (int i = 0; i < inflightFrames.size(); i++)
        {
            VkResult status = vkGetFenceStatus(GPU, inflightFrames[i]->cmdBuffer->fence);
            if (status == VK_SUCCESS) //this command buffer is done executing on the GPU, ready for reuse
            {
                return inflightFrames[i];
            }
        }
        //no command buffers are available
        InflightFrame* NewFrame = CreateInflightFrame();
        inflightFrames.push_back(NewFrame);
        return NewFrame;
    }
}

InflightFrame* GraphicsDevice::CreateInflightFrame()
{
    InflightFrame* frame = new InflightFrame();

    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VULKAN_CALL(vkCreateSemaphore(GPU, &semaphore, nullptr, &frame->imageAvailable));
    VULKAN_CALL(vkCreateSemaphore(GPU, &semaphore, nullptr, &frame->renderFinished));

    frame->cmdBuffer = ImmediateContext->GetCommandBuffer();

    return frame;
}

void GraphicsDevice::initializeMainMemoryManager()
{
    memoryManager = std::make_shared<GPUMemoryManager>(physicalGPU, GPU);
}

void GraphicsDevice::GetGPUProperties()
{
    vkGetPhysicalDeviceProperties(physicalGPU, &gpuProperties);
}

VkPresentModeKHR GraphicsDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

std::array<VkVertexInputAttributeDescription, 2> VertexPositionColor::GetVertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attrDescriptions;
    attrDescriptions[0].binding = 0;
    attrDescriptions[0].location = 0;
    attrDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescriptions[0].offset = offsetof(VertexPositionColor, position);

    attrDescriptions[1].binding = 0;
    attrDescriptions[1].location = 1;
    attrDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescriptions[1].offset = offsetof(VertexPositionColor, color);

    return attrDescriptions;
}

VkVertexInputBindingDescription VertexPositionColor::GetBindingDescription()
{
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(VertexPositionColor);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindDesc;
}

bool QueueFamilyIndices::isComplete()
{
    return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value() && computeFamily.has_value();
}