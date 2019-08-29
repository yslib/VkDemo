#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>


namespace vk
{

}


constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidaionLayers = false;
#endif

struct QueueFamiliyIndices
{
	int graphicsFamiliy = -1;
	int presentFamiliy = -1;
	bool isComplete()const
	{
		return graphicsFamiliy != -1 && presentFamiliy != -1;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SwapChain
{
	VkSwapchainKHR swapChainHandle = nullptr;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	VkSurfaceFormatKHR m_surfaceFormat;
	VkPresentModeKHR m_presentMode;
	VkExtent2D m_extent;
};

VkInstance m_instance = nullptr;
VkPhysicalDevice m_physicalDevice = nullptr;
VkDevice m_device = nullptr;
QueueFamiliyIndices m_queueFamilyIndex;
VkQueue m_graphicQueue;
VkQueue m_presentQueue;

VkDebugUtilsMessengerEXT m_callback;

GLFWwindow* m_window = nullptr;
VkSurfaceKHR m_surface = nullptr;
SwapChain m_swapChain;
VkPipelineLayout m_pipelineLayout;
VkRenderPass m_renderPass;
VkPipeline m_pipeline;
VkCommandPool m_commandPool;
VkSemaphore imageAvailableSmp;
VkSemaphore renderFinishedSmp;



VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//std::cerr << "validation layer: " << pCallbackData->pMessage
	//	<< std::endl;

	//if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	//{
	//	
	//}
	//

	return VK_FALSE;
}



SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface,
			&formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
		&presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
			&presentModeCount, details.presentModes.data());
	}

	return details;
}

std::vector<const char *> GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions,
		glfwExtensions + glfwExtensionCount);
	
	if (enableValidationLayers) 
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const
	VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const
	VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT*
	pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT callback, const
	VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

void setupDebugCallback()
{
	if (enableValidationLayers == false)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional


	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback!");
	}

	
}


bool CheckExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr,
		&extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}


bool CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount,
		availableLayers.data());


	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (layerFound == false) {
			return false;
		}
	}

	return true;
};


bool CreateInstance()
{

	glfwInit();

	if (enableValidationLayers && CheckValidationLayerSupport() == false)
	{
		throw std::runtime_error("validation layers are not available");
	}

	VkApplicationInfo appInfo;
	VkInstanceCreateInfo instanceInfo;

	//vkCreateWin32SurfaceKHR()

	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Vulkan Demo";
	appInfo.applicationVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);


	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nullptr;
	instanceInfo.ppEnabledExtensionNames = nullptr;
	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = nullptr;
	instanceInfo.pApplicationInfo = &appInfo;

	if (enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = validationLayers.size();
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
	}

	// add extensions
	auto extensions = GetRequiredExtensions();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		std::cout << "Cannot create vulkan instance\n";
		return false;
	}
	return true;
}

struct QueueFamiliy
{
	int graphicQueueFamilyIndex = -1;
	bool IsComplete()const { return graphicQueueFamilyIndex != -1; }
};


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_SNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)		// The best choice
		{
			return format;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availabePresentModes)
{
	for (const auto& mode : availabePresentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // The best choice
		{
			return mode;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { WIDTH, HEIGHT };

		actualExtent.width = (std::max)(capabilities.minImageExtent.width,
			(std::min)(capabilities.maxImageExtent.width,
				actualExtent.width));
		actualExtent.height = (std::max)(capabilities.minImageExtent.height,
			(std::min)(capabilities.maxImageExtent.height,
				actualExtent.height));

		return actualExtent;
	}
}



bool IsRequired(VkPhysicalDevice device)
{


	// queue family support
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (auto i = 0; i < queueFamilies.size(); i++)
	{
		const auto& que = queueFamilies[i];
		if (que.queueCount > 0 && que.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_queueFamilyIndex.graphicsFamiliy = i;
		}

		// query the support for surface present queue
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
		if (que.queueCount > 0 && presentSupport)
		{
			m_queueFamilyIndex.presentFamiliy = i;
		}

	}

	if (m_queueFamilyIndex.isComplete() == false)
	{
		throw std::runtime_error("Run Time Error");
		return false;
	}



	// extension check
	auto extension = CheckExtensionSupport(device);

	bool swapChainAdquate = false;
	if (extension)
	{
		const auto swapChainSupport = querySwapChainSupport(device);
		swapChainAdquate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return m_queueFamilyIndex.isComplete() && extension && swapChainAdquate;
}

void GetAPhsicalDevice()
{
	uint32_t dc = 0;
	if (vkEnumeratePhysicalDevices(m_instance, &dc, nullptr) != VK_SUCCESS)
	{
		std::cout << "There is no physical device\n";
	}

	std::vector<VkPhysicalDevice> pds(dc);
	vkEnumeratePhysicalDevices(m_instance, &dc, pds.data());

	VkPhysicalDevice device = nullptr;
	for (const auto& d : pds)
	{
		if (IsRequired(d))
		{
			device = d;
			break;
		}
	}

	m_physicalDevice = device;
}

void CreateLogicalDevice()
{

	// If you want to create a device with more than one queue, modify this

	std::set<int> queueFamilies = { m_queueFamilyIndex.graphicsFamiliy,m_queueFamilyIndex.presentFamiliy };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;


	for (const auto& index : queueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = 1;
		float quePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &quePriority;
		queueCreateInfos.push_back(queueCreateInfo);

	}

	// Setup device features
	VkPhysicalDeviceFeatures supportFeatures;
	VkPhysicalDeviceFeatures requiredFeatures = {};
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportFeatures);
	requiredFeatures.multiDrawIndirect = supportFeatures.multiDrawIndirect;
	requiredFeatures.tessellationShader = VK_TRUE;
	requiredFeatures.geometryShader = VK_TRUE;



	const VkDeviceCreateInfo deviceCreateInfo = {
	VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	nullptr,
	0,
	queueCreateInfos.size(),
	queueCreateInfos.data(),
	0,
	nullptr,
	deviceExtensions.size(),
	deviceExtensions.data(),
	&requiredFeatures };


	if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		throw std::runtime_error("can not create logical device");
		return;
	}

	vkGetDeviceQueue(m_device, m_queueFamilyIndex.graphicsFamiliy, 0, &m_graphicQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndex.presentFamiliy, 0, &m_presentQueue);

}




void CreateSurface()
{
	//glfwCreateWindowSurface();
	
	//glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	auto ret = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
	if (ret != VK_SUCCESS)
	{
		throw std::runtime_error("can not create display surface");
	}

}

void CreateSwapChain()
{

	// First we need to query some necessary information about swap chain
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
	m_swapChain.m_surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	m_swapChain.m_presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	m_swapChain.m_extent = chooseSwapExtent(swapChainSupport.capabilities);

	VkSwapchainCreateInfoKHR createInfo = {};

	// evaluate a proper image count
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	// If maxImageCount == 0, it indicates that you can set the image count as large as possible
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}



	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	createInfo.surface = m_surface;

	createInfo.minImageCount = imageCount;

	createInfo.imageFormat = m_swapChain.m_surfaceFormat.format;

	createInfo.imageColorSpace = m_swapChain.m_surfaceFormat.colorSpace;

	createInfo.imageExtent = m_swapChain.m_extent;

	createInfo.imageArrayLayers = 1;

	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;			// NOTICE

	// If the present queue family is not the same as the graphics queue family, we need take more works
	uint32_t indices[] = { m_queueFamilyIndex.graphicsFamiliy,m_queueFamilyIndex.presentFamiliy };

	if (m_queueFamilyIndex.graphicsFamiliy != m_queueFamilyIndex.presentFamiliy)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;  // One image can be used among several queues concurently.
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = indices;
	}
	else
	{
		// In most cases

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	// One image can be used by one queue at one time.
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = m_swapChain.m_presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;


	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain.swapChainHandle) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain");
	}

	// Get the swap chain images

	// Vulkan could create more image than we expected, so we should also query the actual image count explicitly
	vkGetSwapchainImagesKHR(m_device, m_swapChain.swapChainHandle, &imageCount, nullptr);
	m_swapChain.swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain.swapChainHandle, &imageCount, m_swapChain.swapChainImages.data());

}

void CreateImageViews()
{
	m_swapChain.swapChainImageViews.resize(m_swapChain.swapChainImages.size());

	for(int i =0;i<m_swapChain.swapChainImageViews.size();i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChain.swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChain.m_surfaceFormat.format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &createInfo, nullptr,
			&m_swapChain.swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

std::vector<char> readFile(const std::string & fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);
	if(file.is_open() == false)
	{
		throw std::runtime_error("failed to open file");
	}

	std::vector<char> buffer(file.tellg());
	file.seekg(0);
	file.read(buffer.data(), buffer.size());
	return buffer;
}


VkShaderModule CreateShaderModule(const std::vector<char> & code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode =reinterpret_cast<const uint32_t*> (code.data());

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(m_device,&createInfo,nullptr,&shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader");
	}

	return shaderModule;
	
}



void CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapChain.m_surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear the framebuffer
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // present by the swapchain

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	//subpass.pInputAttachments = nullptr;


	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	/////////////////////////////////////////////////
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	/////////////////////////////////////////////////

	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr,
		&m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

}

void CreateGraphicsPipline()
{
	auto vertShaderCode = readFile(R"(E:\code\vlkdemo\shader\vert.spv)");
	auto fragShaderCode = readFile(R"(E:\code\vlkdemo\shader\frag.spv)");

	auto vertShaderModule = CreateShaderModule(vertShaderCode);
	auto fragShaderModule = CreateShaderModule(fragShaderCode);


	// [Shader Stage]
	// Vertex Shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";		// Manually specified the entry point
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	// Fragment Shader
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";


	VkPipelineShaderStageCreateInfo shaderStages[] =
	{ vertShaderStageInfo, fragShaderStageInfo };


	// [VertexInputState]

	
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// [Input Assembly State]
	// This stage is something like GL_TRIANGLES .etc in OpenGL to specified which primitive to draw
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;


	// [Viewport and scissor]

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChain.m_extent.width;
	viewport.height = (float)m_swapChain.m_extent.height;
	/*The following two variables must be between 0.0 to 1.0 to specified the depth range*/
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = m_swapChain.m_extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	
	// [Rasterization State]
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthBiasClamp = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	// Advanced features
	rasterizer.depthBiasEnable= VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // optional
	rasterizer.depthBiasClamp = 0.0f; //optional
	rasterizer.depthBiasSlopeFactor = 0.0f;//optional

	// [Multisample State]

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // optional
	multisampling.pSampleMask = nullptr;//optional
	multisampling.alphaToCoverageEnable = VK_FALSE;//optional
	multisampling.alphaToOneEnable = VK_FALSE;//optional

	// [Depth and stencil test]

	// [Color and Blending]
	// for every framebuffer attachment
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

	colorBlending.attachmentCount = 1;		// the count of framebuffer
	colorBlending.pAttachments = &colorBlendAttachment;

	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	// [Dynamic State]

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;


	// [Uniforms layout for shader]
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr,
		&m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
	
	// [Pipeline ]
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional

	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if(vkCreateGraphicsPipelines(m_device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline");
	}

	
	
	
	// Shaders in vulkan only used when the pipline was created
	vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

	
}

void CreateFramebuffer()
{
	m_swapChain.swapChainFramebuffers.resize(m_swapChain.swapChainImageViews.size());
	for(int i = 0 ; i < m_swapChain.swapChainImageViews.size();i++)
	{
		VkImageView attachments[] = 
		{
			m_swapChain.swapChainImageViews[i]
		};
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;

		framebufferInfo.width = m_swapChain.m_extent.width;
		framebufferInfo.height = m_swapChain.m_extent.height;
		framebufferInfo.layers = 1;

		if(vkCreateFramebuffer(m_device,&framebufferInfo,nullptr,&m_swapChain.swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer");
		}
	}
	
}

void CreateCommandPool(){
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = m_queueFamilyIndex.graphicsFamiliy;		// for draw call
	createInfo.flags = 0;		// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

	if(vkCreateCommandPool(m_device,&createInfo,nullptr,&m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}
	
}

void CreateCommandBuffer(){
	m_swapChain.commandBuffers.resize(m_swapChain.swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = m_swapChain.commandBuffers.size();
	if(vkAllocateCommandBuffers(m_device,&allocInfo,m_swapChain.commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("railed to allocate command buffers");
	}

	for(size_t i = 0 ;i<m_swapChain.commandBuffers.size();i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if(vkBeginCommandBuffer(m_swapChain.commandBuffers[i],&beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("faild to begin recording command buffer");
		}


		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChain.swapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChain.m_extent;


		VkClearValue clearColor = { 0.f,0.f,0.f,1.f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// [Cmd 1: Begin Render pass]

		vkCmdBeginRenderPass(m_swapChain.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);		// all commands are primary

		// [Cmd 2: Bind pipeline]

		vkCmdBindPipeline(m_swapChain.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);		// Graphics pipeline rather than compute pipeline

		// [Cmd 3: Draw Call]
		vkCmdDraw(m_swapChain.commandBuffers[i], 3, 1, 0, 0);

		// [Cmd 4: End Render pass]
		vkCmdEndRenderPass(m_swapChain.commandBuffers[i]);
		
		if(vkEndCommandBuffer(m_swapChain.commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer");
		}
	}
}

void CreateSemphore()
{
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if(vkCreateSemaphore(m_device,&createInfo,nullptr,&imageAvailableSmp) != VK_SUCCESS ||
		vkCreateSemaphore(m_device,&createInfo,nullptr,&renderFinishedSmp) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores");

	}
}


void InitVulkan()
{
	CreateInstance();
	setupDebugCallback();
	
	CreateSurface();
	GetAPhsicalDevice();
	CreateLogicalDevice();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipline();
	CreateFramebuffer();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSemphore();

	
}


void drawFrame()
{
	uint32_t imageIndex;

	vkAcquireNextImageKHR(m_device, m_swapChain.swapChainHandle, (std::numeric_limits<uint64_t>::max)(), imageAvailableSmp, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {imageAvailableSmp};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_swapChain.commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSmp };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if(vkQueueSubmit(m_graphicQueue,1,&submitInfo,VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}



	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;		// wait the draw call to be done

	VkSwapchainKHR swapChains[] = { m_swapChain.swapChainHandle };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	
	
}

void mainLoop()
{
	while(!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		drawFrame();
	}
}


void CleanupVulkan()
{

	vkDestroySemaphore(m_device, imageAvailableSmp,nullptr);
	vkDestroySemaphore(m_device, renderFinishedSmp,nullptr);

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	// clean up the image view for images in swap chain
	for(auto item:m_swapChain.swapChainImageViews)
	{
		vkDestroyImageView(m_device, item, nullptr);
	}

	for(auto fb:m_swapChain.swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_device, fb, nullptr);
	}
	

	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapChain.swapChainHandle, nullptr);
	vkDestroyDevice(m_device,nullptr);
	vkDestroySurfaceKHR(m_instance,m_surface,nullptr);
	vkDestroyInstance(m_instance, nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
	}
	
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

int main()
{
	InitVulkan();

	mainLoop();
	
	CleanupVulkan();

	return 0;
}