#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>

namespace vk
{
}

namespace{
constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;
constexpr int MAX_FRAMES = 2;


#ifndef __APPLE__
const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
#else
const std::vector<const char*> validationLayers = {};
#endif

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#if !defined(NDEBUG)
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif


std::vector<uint8_t> MakeSPIRV(const std::string& fileName)
{
	using namespace std;
	using namespace std::filesystem;
	using namespace glslang;

	ifstream glsl(fileName);
	if (glsl.is_open() == false)
	{
		cout << "Failed to open glsl file\n";
		throw runtime_error("Failed to open glsl file");
	}
	
	string glslCode{ istreambuf_iterator<char>{glsl}, istreambuf_iterator<char>{} };


	EShLanguage shaderType;

	const string extension = path(fileName).extension().string();

	if(extension =="vert")
	{
		shaderType = EShLangVertex;
	}else if(extension =="tesc")
	{
		shaderType = EShLangTessControl;
	}else if(extension == "tese")
	{
		shaderType = EShLangTessEvaluation;
	}else if(extension == "geom")
	{
		shaderType = EShLangGeometry;
	}else if(extension == "frag")
	{
		shaderType = EShLangFragment;
	}else if(extension == "comp")
	{
		shaderType = EShLangCompute;
	}else
	{
		cout << "Unknown shader type: "<<extension<<endl;
		return {};
	}

	TShader shader(shaderType);

	const char* code = glslCode.c_str();
	shader.setStrings(&code, 1);

	int ClientInputSemanticsVersion = 100;
	EShTargetClientVersion vkVersion = EShTargetVulkan_1_1;
	EShTargetLanguageVersion spirvVersion = EShTargetSpv_1_0;

	shader.setEnvInput(EShSourceGlsl, shaderType, EShClientVulkan, ClientInputSemanticsVersion);
	shader.setEnvClient(EShClientVulkan, vkVersion);
	shader.setEnvTarget(EshTargetSpv, spirvVersion);


	EShMessages msg = (EShMessages)(EShMsgSpvRules|EShMsgVulkanRules);

	const int defaultVersion = 100;

	DirStackFileIncluder includer;

	const string p = path(fileName).parent_path().string();
	includer.pushExternalLocalDirectory(p);

	string preprocessedGLSL;

	if(shader.preprocess(nullptr,defaultVersion,ENoProfile,false,false,msg,&preprocessedGLSL,includer))
	{
		cout << "GLSL preprocessing failed for: " << fileName << endl;
		cout << shader.getInfoLog() << endl;
		cout << shader.getInfoDebugLog() << endl;
	}

	const char* preproccesdGLSLStr = preprocessedGLSL.c_str();
	shader.setStrings(&preproccesdGLSLStr,1);

	
	
	return {};
}


struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct QueueFamiliyIndices
{
	uint32_t graphicsFamiliy = -1;
	uint32_t presentFamiliy = -1;
	bool isComplete() const
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

struct QueueFamiliy
{
	int graphicQueueFamilyIndex = -1;
	bool IsComplete() const { return graphicQueueFamilyIndex != -1; }
};


struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription
		GetBindingDescription()
	{
		VkVertexInputBindingDescription binding = {};

		binding.binding = 0;
		binding.stride = sizeof(Vertex);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding;
	}

	static std::array<VkVertexInputAttributeDescription, 2>
		GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> desc;

		desc[0].binding = 0;
		desc[0].location = 0;
		desc[0].format = VK_FORMAT_R32G32_SFLOAT;
		desc[0].offset = offsetof(Vertex, pos);

		//  float��VK_FORMAT_R32_SFLOAT
		//  vec2��VK_FORMAT_R32G32_SFLOAT
		//	vec3��VK_FORMAT_R32G32B32_SFLOAT
		//	vec4��VK_FORMAT_R32G32B32A32_SFLOAT
		desc[1].binding = 0;
		desc[1].location = 1;
		desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc[1].offset = offsetof(Vertex, color);




		return desc;
	}

};

//const std::vector<Vertex> vertices = {
//	{{.0f,-.5f},{1.f,1.f,1.f}},
//	{{.5f,.5f},{0.f,1.f,0.f}},
//	{{-0.5f,.5f},{0.f,0.f,1.f}}
//};
const std::vector<Vertex> vertices = {
	{{-0.5f,-0.5f},{1.0f,0.0f,0.0f}},
{{0.5f,-0.5f},{0.f,1.0f,0.0f}},
{{0.5f,0.5f},{0.f,0.f,1.f}},
{{-0.5f,0.5f},{1.f,1.f,1.f}}
};



const std::vector < uint16_t > indices = {
0,1,2,2,3,0 };



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
VkBuffer m_vertexBuffer;
VkDeviceMemory m_vertexBufferMemory;
VkBuffer m_indexBuffer;
VkDeviceMemory m_indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
VkDescriptorPool m_descPool;
VkDescriptorSetLayout m_descriptorSetLayout; // ubo 
std::vector<VkDescriptorSet> m_descSets;

size_t m_currentFrame = 0;


//VkSemaphore imageAvailableSmp;
//VkSemaphore renderFinishedSmp;

std::vector<VkSemaphore> imageAvaliableSmps;
std::vector<VkSemaphore> renderFinishedSmps;
std::vector<VkFence> fences;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

	//std::cerr << "validation layer: " << pCallbackData->pMessage
	//	<< std::endl;

	//if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	//{
	//
	//}
	//

	return VK_FALSE;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
std::vector<const char*> GetRequiredExtensions();
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);

void setupDebugCallback();

bool CheckExtensionSupport(VkPhysicalDevice device);
bool CheckValidationLayerSupport();
bool CreateInstance();
void CreateLogicalDevice();

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availabePresentModes);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availabePresentModes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
bool IsRequired(VkPhysicalDevice device);
void GetAPhsicalDevice();
void CreateSurface();
void CreateSwapChain();
void CreateImageViews();
std::vector<char> readFile(const std::string& fileName);
VkShaderModule CreateShaderModule(const std::vector<char>& code);
void CreateRenderPass();
void CreateGraphicsPipline();
void CreateFramebuffer();
void CreateCommandPool();
void CreateCommandBuffer();
void CreateSemphore();

void RecreateSwapChain();
void CleanupSwapChain();
void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void CreateVertexBuffer();
void CreateIndexBuffer();
void CreateDescriptorSetLayout();
void CreateUniformBuffers();
void UpdateUniformBuffer(uint32_t curImage);
void CreateDescriptorPool();
void CreateDescriptorSets();
uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);;



/////////////////////////////

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

std::vector<const char*> GetRequiredExtensions()
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

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
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

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (layerFound == false)
		{
			return false;
		}
	}

	return true;
}

bool CreateInstance()
{

	if (!glfwInit())
	{
		throw std::runtime_error("failed to init glfw");
	}

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
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nullptr;
	instanceInfo.ppEnabledExtensionNames = nullptr;
	instanceInfo.enabledExtensionCount = 0;
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
	std::cout << "Extensions:" << std::endl;
	for (const auto& e : extensions)
	{
		std::cout << e << std::endl;
	}
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	auto ret = vkCreateInstance(&instanceInfo, nullptr, &m_instance);

	std::cout << ret << std::endl;
	if (ret != VK_SUCCESS)
	{
		throw std::runtime_error("Cannot create vulkan instance");
	}
	return true;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_SNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // The best choice
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
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;

		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };

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

	std::set<uint32_t> queueFamilies = { m_queueFamilyIndex.graphicsFamiliy, m_queueFamilyIndex.presentFamiliy };

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
	//requiredFeatures.geometryShader = VK_TRUE;

	const VkDeviceCreateInfo deviceCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>(queueCreateInfos.size()),
		(queueCreateInfos.data()),
		0,
		nullptr,
		static_cast<uint32_t>(deviceExtensions.size()),
		(deviceExtensions.data()),
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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // NOTICE

	// If the present queue family is not the same as the graphics queue family, we need take more works
	uint32_t indices[] = { m_queueFamilyIndex.graphicsFamiliy, m_queueFamilyIndex.presentFamiliy };

	if (m_queueFamilyIndex.graphicsFamiliy != m_queueFamilyIndex.presentFamiliy)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // One image can be used among several queues concurently.
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = indices;
	}
	else
	{
		// In most cases

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // One image can be used by one queue at one time.
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

	for (int i = 0; i < m_swapChain.swapChainImageViews.size(); i++)
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
			&m_swapChain.swapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

std::vector<char> readFile(const std::string& fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);
	if (file.is_open() == false)
	{
		throw std::runtime_error("failed to open file");
	}
	std::vector<char> buffer(file.tellg());
	file.seekg(0);
	file.read(buffer.data(), buffer.size());
	return buffer;
}

VkShaderModule CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
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

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // clear the framebuffer
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
		&m_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void CreateGraphicsPipline()
{
	auto vertShaderCode = readFile(R"(shader/vert.spv)");
	auto fragShaderCode = readFile(R"(shader/frag.spv)");

	auto vertShaderModule = CreateShaderModule(vertShaderCode);
	auto fragShaderModule = CreateShaderModule(fragShaderCode);

	// [Shader Stage]
	// Vertex Shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; // Manually specified the entry point
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

	auto bindingDesc = Vertex::GetBindingDescription();
	auto attrDesc = Vertex::GetAttributeDescriptions();


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());;
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data(); // Optional

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
	scissor.offset = { 0, 0 };
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
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // optional
	rasterizer.depthBiasClamp = 0.0f;		   //optional
	rasterizer.depthBiasSlopeFactor = 0.0f;	//optional

	// [Multisample State]

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	multisampling.minSampleShading = 1.0f;			   // optional
	multisampling.pSampleMask = nullptr;			   // optional
	multisampling.alphaToCoverageEnable = VK_FALSE;    // optional
	multisampling.alphaToOneEnable = VK_FALSE;		   // optional

	// [Depth and stencil test]

	// [Color and Blending]
	// for every framebuffer attachment
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;			 // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;			 // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

	colorBlending.attachmentCount = 1; // the count of framebuffer
	colorBlending.pAttachments = &colorBlendAttachment;

	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// [Dynamic State]

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// [Uniforms layout for shader]
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;			  // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr;		  // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0;	// Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr,&m_pipelineLayout) != VK_SUCCESS)
	{
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

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
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
	for (int i = 0; i < m_swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] =
		{
			m_swapChain.swapChainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;

		framebufferInfo.width = m_swapChain.m_extent.width;
		framebufferInfo.height = m_swapChain.m_extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChain.swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void CreateCommandPool()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = m_queueFamilyIndex.graphicsFamiliy; // for draw call
	createInfo.flags = 0;											  // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

	if (vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}
}

void CreateCommandBuffer()
{
	m_swapChain.commandBuffers.resize(m_swapChain.swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = m_swapChain.commandBuffers.size();
	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_swapChain.commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("railed to allocate command buffers");
	}

	for (size_t i = 0; i < m_swapChain.commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_swapChain.commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("faild to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChain.swapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChain.m_extent;

		VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// [Cmd 1: Begin Render pass]

		vkCmdBeginRenderPass(m_swapChain.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // all commands are primary

		// [Cmd 2: Bind pipeline]

		vkCmdBindPipeline(m_swapChain.commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline); // Graphics pipeline rather than compute pipeline

		// [Cmd 3: Bind Vertex Buffer]
		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets = { 0 };
		vkCmdBindVertexBuffers(m_swapChain.commandBuffers[i], 0, 1, vertexBuffers, &offsets);

		vkCmdBindIndexBuffer(m_swapChain.commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);


		// [Cmd 3: Draw Call]
		//vkCmdDraw(m_swapChain.commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		vkCmdBindDescriptorSets(
			m_swapChain.commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			0,
			1,
			&m_descSets[i],
			0,
			nullptr);
		vkCmdDrawIndexed(m_swapChain.commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// [Cmd 4: End Render pass]
		vkCmdEndRenderPass(m_swapChain.commandBuffers[i]);

		if (vkEndCommandBuffer(m_swapChain.commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer");
		}
	}
}

void CreateSemphore()
{
	imageAvaliableSmps.resize(MAX_FRAMES);
	renderFinishedSmps.resize(MAX_FRAMES);
	fences.resize(MAX_FRAMES);

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES; i++) {

		if (vkCreateSemaphore(m_device, &createInfo, nullptr, &imageAvaliableSmps[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &createInfo, nullptr, &renderFinishedSmps[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fenceInfo, nullptr, &fences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores");
		}

	}
}


void RecreateSwapChain() {

	vkDeviceWaitIdle(m_device);

	CleanupSwapChain();

	CreateSwapChain();

	CreateImageViews();

	CreateRenderPass();

	CreateGraphicsPipline();

	CreateFramebuffer();

	CreateCommandBuffer();

}


void CleanupSwapChain() {

	for (auto item : m_swapChain.swapChainFramebuffers)
		vkDestroyFramebuffer(m_device, item, nullptr);

	vkFreeCommandBuffers(m_device, m_commandPool, m_swapChain.commandBuffers.size(), m_swapChain.commandBuffers.data());

	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	for (auto item : m_swapChain.swapChainImageViews)
		vkDestroyImageView(m_device, item, nullptr);

	vkDestroySwapchainKHR(m_device, m_swapChain.swapChainHandle, nullptr);

}

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProp;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProp);

	for (uint32_t i = 0; i < i < memProp.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProp.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = { };

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);


	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(m_device, buffer, memory, 0);

}

void CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingbuffer,
		stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), size_t(bufferSize));
	vkUnmapMemory(m_device, stagingBufferMemory);

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer,
		m_indexBufferMemory);


	CopyBuffer(stagingbuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingbuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);

}
void CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,		// ?????
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingbuffer,
		stagingBufferMemory);

	void* m_data = nullptr;
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &m_data);
	memcpy(m_data, vertices.data(), size_t(bufferSize));
	vkUnmapMemory(m_device, stagingBufferMemory);

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMemory);


	CopyBuffer(stagingbuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingbuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);

}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  // notice

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(m_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicQueue); // wait to be done

	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);

}

void CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;		//only for vertex shader

	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
	//VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
}

void CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	const auto count = m_swapChain.swapChainImages.size();

	uniformBuffers.resize(m_swapChain.swapChainImages.size());
	uniformBuffersMemory.resize(uniformBuffers.size());

	for (size_t i = 0; i < count; i++)
	{
		CreateBuffer(bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
	}
}


void UpdateUniformBuffer(uint32_t curImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {  };
	ubo.model = glm::rotate(glm::mat4(1.0), time * glm::radians(90.0f), glm::vec3(0, 0, 1));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f),
		m_swapChain.m_extent.width / (float)m_swapChain.m_extent.height,
		0.1f, 10.0f);

	//NOTE: The direction of y-axis of the scissor coordinate of Vulkan is opposite from OpenGL 
	ubo.proj[1][1] *= -1;


	//copy data into buffer memory

	void* data;
	vkMapMemory(m_device, uniformBuffersMemory[curImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_device, uniformBuffersMemory[curImage]);


}

void CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_swapChain.swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	poolInfo.maxSets = static_cast<uint32_t>(m_swapChain.swapChainImages.size());

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool");
	}

}

void CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_swapChain.swapChainImages.size(),
		m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChain.swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	m_descSets.resize(m_swapChain.swapChainImages.size());

	if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descSets[0]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < m_swapChain.swapChainImages.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);
		//bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descWrite = {};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = m_descSets[i];
		descWrite.dstBinding = 0;
		descWrite.dstArrayElement = 0;
		
		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrite.descriptorCount = 1;
		
		descWrite.pBufferInfo = &bufferInfo;
		descWrite.pImageInfo = nullptr;
		descWrite.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(m_device, 1, &descWrite, 0, nullptr);
	}

}


void InitVulkan()
{
	CreateInstance();

	std::cout << "create instance success\n";

	setupDebugCallback();

	std::cout << "setup debug callbakc success\n";

	CreateSurface();

	std::cout << "create surface success\n";

	GetAPhsicalDevice();

	std::cout << "enumerate a propert physical devcie success\n";

	CreateLogicalDevice();

	std::cout << "create logical device success\n";


	CreateSwapChain();

	std::cout << "create swapchain success\n";

	CreateImageViews();

	std::cout << "create swapchain image view success\n";

	CreateRenderPass();

	CreateDescriptorSetLayout();

	CreateGraphicsPipline();

	CreateFramebuffer();

	CreateCommandPool();


	CreateUniformBuffers();

	CreateDescriptorPool();

	CreateDescriptorSets();
	
	CreateVertexBuffer();
	CreateIndexBuffer();

	CreateCommandBuffer();

	CreateSemphore();
}

void drawFrame()
{
	vkWaitForFences(m_device, 1, &fences[m_currentFrame], VK_TRUE, (std::numeric_limits<uint64_t>::max)());


	uint32_t imageIndex;
	auto result = vkAcquireNextImageKHR(m_device, m_swapChain.swapChainHandle, (std::numeric_limits<uint64_t>::max)(), imageAvaliableSmps[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image");
	}


	std::cout << "Image Index:" << imageIndex << std::endl;
	UpdateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvaliableSmps[m_currentFrame] };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_swapChain.commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSmps[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_device, 1, &fences[m_currentFrame]);
	if (vkQueueSubmit(m_graphicQueue, 1, &submitInfo, fences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores; // wait the draw call to be done

	VkSwapchainKHR swapChains[] = { m_swapChain.swapChainHandle };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(m_presentQueue); ///???

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES;
}

void mainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		drawFrame();
	}
}

void CleanupVulkan()
{
	CleanupSwapChain();

	vkDestroyDescriptorPool(m_device, m_descPool, nullptr);

	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);


	for (size_t i = 0; i < m_swapChain.swapChainImages.size(); i++)
	{
		vkDestroyBuffer(m_device, uniformBuffers[i], nullptr);
		vkFreeMemory(m_device, uniformBuffersMemory[i], nullptr);
	}


	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);

	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);

	// sync objects
	for (int i = 0; i < MAX_FRAMES; i++)
	{
		vkDestroySemaphore(m_device, imageAvaliableSmps[i], nullptr);
		vkDestroySemaphore(m_device, renderFinishedSmps[i], nullptr);
		vkDestroyFence(m_device, fences[i], nullptr);
	}

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);



	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
	}

	glfwDestroyWindow(m_window);
	glfwTerminate();
}
}
int main2()
{
	InitVulkan();

	mainLoop();

	CleanupVulkan();

	return 0;
}
