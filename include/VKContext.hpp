#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>
#include <cstring>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef __linux__
#define VK_USE_PLATFORM_XCB_KHR
//#define VK_USE_PLATFORM_XLIB_KHR
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

#ifndef __APPLE__
const std::vector<const char *> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
#else
const std::vector<const char *> validationLayers = {};
#endif

const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#if !defined( NDEBUG )
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

using FramebufferResizeEventCallback = std::function<void( void *, int width, int height )>;
//
#define VK_CHECK( expr )                                                                       \
	do {                                                                                       \
		VkResult res;                                                                          \
		if ( ( res = expr ) != VK_SUCCESS ) {                                                  \
			std::cout << "VkResult(" << res << "): " << #expr << ": Vulkan Assertion Failed:"; \
			exit( -1 );                                                                        \
		}                                                                                      \
	} while ( false );
VkAllocationCallbacks *g_allocation = nullptr;

std::vector<const char *> GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
	std::vector<const char *> extensions( glfwExtensions,
										  glfwExtensions + glfwExtensionCount );

	if ( enableValidationLayers ) {
		extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	}

	return extensions;
}

struct VkInstanceObject
{
	VkInstanceObject()
	{
		uint32_t extPropCount = 0;
		vkEnumerateInstanceExtensionProperties( nullptr, &extPropCount, nullptr );
		std::vector<VkExtensionProperties> props( extPropCount );
		vkEnumerateInstanceExtensionProperties( nullptr, &extPropCount, props.data() );
		for ( int i = 0; i < props.size(); i++ ) {
			std::cout << props[ i ].extensionName << std::endl;
		}
		if ( glfwInit() != GLFW_TRUE ) {
			std::cout << "glfw init failed\n";
			exit( -1 );
		};
		uint32_t extCount = 0;
		auto ext = glfwGetRequiredInstanceExtensions( &extCount );
		std::vector<const char *> extNames( ext, ext + extCount );
		for ( const auto &c : extNames ) {
			std::cout << "Extension: " << c << std::endl;
		}

		if ( enableValidationLayers ) {
			extNames.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}

		VkInstanceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CI.pNext = nullptr;
		CI.enabledLayerCount = 0;
		CI.ppEnabledLayerNames = nullptr;

		CI.enabledExtensionCount = extNames.size();
		CI.ppEnabledExtensionNames = extNames.data();

		CI.pApplicationInfo = nullptr;
		////Check validationLayers
		if ( false ) {	// warning: If layer is enabled, the VK_LAYER_PATH must be set
			CI.enabledLayerCount = validationLayers.size();
			CI.ppEnabledLayerNames = validationLayers.data();
		}

		VK_CHECK( vkCreateInstance( &CI, AllocationCallback, &vkInstance ) );
#ifndef NDEBUG
		if ( enableValidationLayers ) {
			VkDebugUtilsMessengerCreateInfoEXT ci = {};
			ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			ci.pNext = nullptr;
			ci.pUserData = nullptr;
			ci.pfnUserCallback = debugCallback;
			ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			debugUtilsMessenger = CreateDebugUtilsMessenger( ci );
			if ( debugUtilsMessenger == VK_NULL_HANDLE ) {
				std::cout << "Create Debug Utils messenger failed\n";
				exit( -1 );
			}
		}
#endif
	}
	~VkInstanceObject()
	{
		DestroyDebugUtilsMessenger( debugUtilsMessenger );
		vkDestroyInstance( vkInstance, AllocationCallback );
	}
	operator VkInstance()
	{
		return vkInstance;
	}
	VkAllocationCallbacks *AllocationCallback = nullptr;

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	  void *pUserData )
	{

    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}
	VkDebugUtilsMessengerEXT
	  CreateDebugUtilsMessenger( const VkDebugUtilsMessengerCreateInfoEXT &ci )
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		  vkInstance, "vkCreateDebugUtilsMessengerEXT" );
		VkDebugUtilsMessengerEXT vkHandle = VK_NULL_HANDLE;
		if ( func ) {
			VK_CHECK( func( vkInstance, &ci, AllocationCallback, &vkHandle ) );
		} else {
			std::cout << "Failed to load vkCreateDebugUtilsMessengerEXT\n";
			exit( -1 );
		}
		return vkHandle;
	}
	void DestroyDebugUtilsMessenger( VkDebugUtilsMessengerEXT object )
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( vkInstance, "vkDestroyDebugUtilsMessengerEXT" );
		if ( func ) {
			func( vkInstance, object, AllocationCallback );
		} else {
			std::cout << "Failed to load vkDestoryDebugUtilsMessengerEXT\n";
		}
	}

	VkInstance vkInstance;
	VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
};

struct VkPhysicalDeviceObject
{
	std::shared_ptr<VkInstanceObject> Instance;
	std::vector<VkMemoryType> AllowedMemoryType;
	std::vector<VkQueueFamilyProperties> QueueFamilies;
	VkPhysicalDeviceFeatures PhysicalDeviceFeatures;

	VkPhysicalDeviceObject( std::shared_ptr<VkInstanceObject> instance ) :
	  Instance( std::move( instance ) )
	{
		uint32_t count = 0;
		VK_CHECK( vkEnumeratePhysicalDevices( *Instance, &count, nullptr ) );
		using namespace std;
		vector<VkPhysicalDevice> dev( count );
		VK_CHECK( vkEnumeratePhysicalDevices( *Instance, &count, dev.data() ) );
		std::cout << "There are " << count << " physical device(s)\n";

		for ( int i = 0; i < count; i++ ) {
			VkPhysicalDeviceProperties prop;
			auto d = dev[ i ];
			vkGetPhysicalDeviceProperties( d, &prop );
			VkPhysicalDeviceMemoryProperties memProp;
			vkGetPhysicalDeviceMemoryProperties( d, &memProp );
			for ( int i = 0; i < memProp.memoryTypeCount; i++ ) {
				AllowedMemoryType.push_back( memProp.memoryTypes[ i ] );
			}

			//vkGetPhysicalDeviceProperties(d,&prop);
			//vkGetPhysicalDeviceFeatures(d, &feat);
			// We don't do any assumption for device, just choose the first physical device
			PhysicalDevice = d;

			// Get queue families
			uint32_t count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &count, nullptr );
			assert( count > 0 );
			std::vector<VkQueueFamilyProperties> que( count );
			vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &count, que.data() );

			// Get PhysicalDeviceFeatures
			vkGetPhysicalDeviceFeatures( PhysicalDevice, &PhysicalDeviceFeatures );

			break;
		}
	}

	VkPhysicalDeviceObject( const VkPhysicalDeviceObject & ) = delete;
	VkPhysicalDeviceObject &operator=( const VkPhysicalDeviceObject & ) = delete;
	VkPhysicalDeviceObject( VkPhysicalDeviceObject && ) noexcept = default;
	VkPhysicalDeviceObject &operator=( VkPhysicalDeviceObject && ) noexcept = default;

	operator VkPhysicalDevice()
	{
		return PhysicalDevice;
	}
	uint32_t FindProperMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags flags )
	{
		for ( int i = 0; i < AllowedMemoryType.size(); i++ ) {
			if ( ( typeFilter & ( 1 << i ) ) && ( AllowedMemoryType[ i ].propertyFlags & flags ) == flags ) {
				return AllowedMemoryType[ i ].heapIndex;
			}
		}
		assert( false && "Failed to find proper memory type" );
		return ~0;
	}

	VkFormat FindSupportedFormat( const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features )
	{
		for ( VkFormat format : candidates ) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties( PhysicalDevice, format, &props );
			if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features ) {
				return format;
			} else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features ) {
				return format;
			}
		}
		std::cout << "Unsupported format\n";
		exit( -1 );
	}
	VkSampleCountFlagBits GetMaxUsableSampleCount()
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties( PhysicalDevice, &props );

		VkSampleCountFlags counts = std::min( props.limits.framebufferColorSampleCounts,
											  props.limits.framebufferDepthSampleCounts );
		if ( counts & VK_SAMPLE_COUNT_64_BIT ) { return VK_SAMPLE_COUNT_64_BIT; }
		if ( counts & VK_SAMPLE_COUNT_32_BIT ) { return VK_SAMPLE_COUNT_32_BIT; }
		if ( counts & VK_SAMPLE_COUNT_16_BIT ) { return VK_SAMPLE_COUNT_16_BIT; }
		if ( counts & VK_SAMPLE_COUNT_8_BIT ) { return VK_SAMPLE_COUNT_8_BIT; }
		if ( counts & VK_SAMPLE_COUNT_4_BIT ) { return VK_SAMPLE_COUNT_4_BIT; }
		if ( counts & VK_SAMPLE_COUNT_2_BIT ) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	const VkPhysicalDeviceFeatures &GetPhysicalDeviceFeatures()
	{
		return PhysicalDeviceFeatures;
	}

	int GetQueueIndex( VkQueueFlagBits queueFlags ) const
	{
		const auto count = QueueFamilies.size();
		for ( int i = 0; i < count; i++ ) {
			const auto q = QueueFamilies[ i ];
			if ( q.queueCount > 0 && q.queueFlags & queueFlags ) {
				return i;
			}
		}
		std::cout << "specified queue is not found\n";
		return -1;
	}

	VkBool32 SurfaceSupport() const
	{
		VkBool32 support = VK_FALSE;
		//vkGetPhysicalDeviceSurfaceSupportKHR( *(PhysicalDevice ), GraphicsQueueIndex, Surface, &support );
		return support;
	}

	VkFormat FindDepthFormat()
	{
		return FindSupportedFormat(
		  { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		  VK_IMAGE_TILING_OPTIMAL,
		  VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
	}

	bool hasDepthStencil( VkFormat format )
	{
		return ( format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT );
	}

private:
	VkPhysicalDevice PhysicalDevice;
};

struct VkDeviceObject;

template <typename VkObjectType>
struct VkObject
{
	VkObject() = default;
	void Release();
	VkObject( const VkObject & ) = delete;
	VkObject &operator=( const VkObject & ) = delete;

	VkObject( VkObject &&rhs ) noexcept;
	VkObject &operator=( VkObject &&rhs ) noexcept;

	bool Valid() const { return Object != VK_NULL_HANDLE; }

	operator VkObjectType() { return Object; }
	~VkObject() { Release(); }

	friend std::ofstream &operator<<( std::ofstream &os, const VkObject &object )
	{
		os << "[Object: " << object.Object << ", from Device: " << object.Device << "]\n";
		return os;
	}

protected:
	std::shared_ptr<VkDeviceObject> Device;
	friend class VkDeviceObject;
	VkObject( std::shared_ptr<VkDeviceObject> device, VkObjectType vkObject ) :
	  Device( std::move( device ) ), Object( vkObject ) {}
	VkObjectType Object;
};

using VkBufferObjectBase = VkObject<VkBuffer>;
using VkBufferViewObject = VkObject<VkBufferView>;
using VkImageObjectBase = VkObject<VkImage>;

using VkImageViewObject = VkObject<VkImageView>;
using VkSwapchainObjectBase = VkObject<VkSwapchainKHR>;
using VkPipelineObject = VkObject<VkPipeline>;
using VkShaderModuleObject = VkObject<VkShaderModule>;
using VkPipelineLayoutObject = VkObject<VkPipelineLayout>;
using VkRenderPassObject = VkObject<VkRenderPass>;
using VkFramebufferObject = VkObject<VkFramebuffer>;
using VkCommandPoolObject = VkObject<VkCommandPool>;
using VkCommandBufferObject = VkObject<VkCommandBuffer>;
using VkSemaphoreObject = VkObject<VkSemaphore>;
using VkFenceObject = VkObject<VkFence>;
using VkDeviceMemoryObject = VkObject<VkDeviceMemory>;
using VkDescriptorSetLayoutObject = VkObject<VkDescriptorSetLayout>;
using VkDescriptorPoolObject = VkObject<VkDescriptorPool>;
using VkSamplerObject = VkObject<VkSampler>;

struct VkSurfaceObject;

struct VkBufferObject : public VkBufferObjectBase
{
	std::shared_ptr<VkDeviceMemoryObject> Memory;
	VkBufferCreateInfo Desc;
	VkBufferObject() = default;
	VkBufferObject( std::shared_ptr<VkDeviceObject> device,
					VkBuffer object ) :
	  VkBufferObjectBase( device, object ) {}
	void SetData( const void *data, size_t size );
};

struct VkImageObject : public VkImageObjectBase
{
	std::shared_ptr<VkDeviceMemoryObject> Memory;
	VkImageCreateInfo Desc;
	VkImageObject() = default;
	VkImageObject( std::shared_ptr<VkDeviceObject> device, VkImage object ) :
	  VkImageObjectBase( std::move( device ), object ) {}
	VkImageViewObject CreateImageView( VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags );
};

struct VkSwapchainObject : public VkSwapchainObjectBase
{
	using BaseType = VkSwapchainObjectBase;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageViewObject> SwapchainImageViews;
	std::shared_ptr<VkSurfaceObject> Surface;
	VkSurfaceCapabilitiesKHR cap;
	VkSurfaceFormatKHR SurfaceFormat;
	VkExtent2D Size;
	static FramebufferResizeEventCallback Callback;

	VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, std::shared_ptr<VkSurfaceObject> surface );
	void Update()
	{
		*this = VkSwapchainObject( Device, Surface );
	}
	void SetResizeCallback( FramebufferResizeEventCallback callback )
	{
		Callback = std::move( callback );
	}

private:
	static void glfwWindowResizeCallback( GLFWwindow *window, int width, int height )
	{
		Callback( nullptr, width, height );
	}
	friend class VkDeviceObject;
	VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, VkSwapchainKHR vkHandle );
};
FramebufferResizeEventCallback VkSwapchainObject::Callback = FramebufferResizeEventCallback();
//using VkSwapchainObject = VkObject<VkSwapchainKHR>;

struct VkDeviceObject : public std::enable_shared_from_this<VkDeviceObject>
{
	//std::unordered_map<uint32_t,std::shared_ptr<VkDeviceMemoryObject>> Memory;
	VkDeviceObject( std::shared_ptr<VkPhysicalDeviceObject> physicalDevice ) :
	  PhysicalDevice( std::move( physicalDevice ) )
	{
		VkDeviceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		CI.pNext = nullptr;

		GraphicsQueueIndex = PhysicalDevice->GetQueueIndex( VK_QUEUE_GRAPHICS_BIT );

		VkDeviceQueueCreateInfo queCI = {};
		queCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queCI.pNext = nullptr;
		queCI.queueFamilyIndex = GraphicsQueueIndex;
		queCI.queueCount = 1;
		float pri = 1.0;
		queCI.pQueuePriorities = &pri;

		VkPhysicalDeviceFeatures feat = {};
		feat = PhysicalDevice->GetPhysicalDeviceFeatures();
		feat.samplerAnisotropy = VK_TRUE;
		feat.sampleRateShading = VK_TRUE;

		CI.pQueueCreateInfos = &queCI;
		CI.queueCreateInfoCount = 1;

		CI.enabledLayerCount = validationLayers.size();
		CI.ppEnabledLayerNames = validationLayers.data();

		CI.ppEnabledExtensionNames = deviceExtensions.data();  // for swap chain
		CI.enabledExtensionCount = deviceExtensions.size();

		CI.pEnabledFeatures = &feat;

		VK_CHECK( vkCreateDevice( *PhysicalDevice, &CI, PhysicalDevice->Instance->AllocationCallback, &Device ) );

		vkGetDeviceQueue( Device, GraphicsQueueIndex, 0, &GraphicsQueue );

		// Allocate Memory
		// for(int i = 0 ; i < physicaldevice->allowedmemorytype.size();i++){
		// vkmemoryallocateinfo ci={
		// vk_structure_type_memory_allocate_info,
		// nullptr,
		// 100*1024*1024, // 100 mb
		// physicaldevice->allowedmemorytype[i].heapindex
		// };
		// memory[physicaldevice->allowedmemorytype[i].heapindex]=createdevicememory(ci);
		// }

		// Get Queue
	}
	~VkDeviceObject()
	{
		vkDestroyDevice( Device, PhysicalDevice->Instance->AllocationCallback );
	}

	VkDeviceObject( const VkDeviceObject & ) = delete;
	VkDeviceObject &operator=( const VkDeviceObject & ) = delete;

	VkDeviceObject( VkDeviceObject &&rhs ) noexcept :
	  PhysicalDevice( std::move( rhs.PhysicalDevice ) ), Device( rhs.Device )
	{
		rhs.Device = VK_NULL_HANDLE;
	}

	VkDeviceObject &operator=( VkDeviceObject &&rhs ) noexcept
	{
		vkDestroyDevice( Device, PhysicalDevice->Instance->AllocationCallback );
		PhysicalDevice = std::move( rhs.PhysicalDevice );
		Device = rhs.Device;
		rhs.Device = VK_NULL_HANDLE;
		return *this;
	}
	operator VkDevice()
	{
		return Device;
	}
	operator VkDevice() const
	{
		return Device;
	}

	VkSwapchainObject CreateSwapchain( const VkSwapchainCreateInfoKHR &CI )
	{
		VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateSwapchainKHR( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkSwapchainObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkSwapchainObjectBase &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroySwapchainKHR( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkImageObject CreateImage( const VkImageCreateInfo &CI )
	{
		VkImage vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateImage( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		//VkDeviceSize size = CI.extent.width * CI.extent.height * CI.extent.depth;
		auto imageObject = VkImageObject( this->shared_from_this(), vkHandle );
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements( Device, imageObject, &req );
		auto type = PhysicalDevice->FindProperMemoryType( req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VkMemoryAllocateInfo allocInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			req.size,
			type
		};
		imageObject.Memory = CreateDeviceMemory( allocInfo );
		imageObject.Desc = CI;
		vkBindImageMemory( Device, imageObject, *imageObject.Memory, 0 );
		return std::move( imageObject );
	}

	void DeleteObject( VkImageObjectBase &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyImage( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkImageViewObject CreateImageView( const VkImageViewCreateInfo &CI )
	{
		VkImageView vkHandle = VK_NULL_HANDLE;
		vkCreateImageView( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkImageViewObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkImageViewObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyImageView( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}
	VkBufferObject CreateBuffer( const VkBufferCreateInfo &CI, VkMemoryPropertyFlags flags )
	{
		VkBuffer vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateBuffer( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		auto bufferObject = VkBufferObject( this->shared_from_this(), vkHandle );
		VkMemoryRequirements req = {};
		vkGetBufferMemoryRequirements( Device, bufferObject, &req );
		auto type = PhysicalDevice->FindProperMemoryType( req.memoryTypeBits, flags );
		VkMemoryAllocateInfo allocInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			req.size,
			type
		};
		auto memory = CreateDeviceMemory( allocInfo );
		VK_CHECK( vkBindBufferMemory( Device, bufferObject, *memory, 0 ) );
		bufferObject.Memory = std::move( memory );
		bufferObject.Desc = CI;
		return bufferObject;
	}

	void DeleteObject( VkBufferObjectBase &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyBuffer( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkBufferViewObject CreateBufferView( const VkBufferViewCreateInfo &CI )
	{
		VkBufferView vkHandle = VK_NULL_HANDLE;
		vkCreateBufferView( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkBufferViewObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkBufferViewObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyBufferView( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkPipelineObject CreatePipeline( const VkGraphicsPipelineCreateInfo &CI )
	{
		VkPipeline vkHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines( Device, VK_NULL_HANDLE, 1, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkPipelineObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkPipelineObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyPipeline( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkShaderModuleObject CreateShader( const VkShaderModuleCreateInfo &CI )
	{
		VkShaderModule vkHandle = VK_NULL_HANDLE;
		vkCreateShaderModule( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkShaderModuleObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkShaderModuleObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyShaderModule( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkPipelineLayoutObject CreatePipelineLayout( const VkPipelineLayoutCreateInfo &CI )
	{
		VkPipelineLayout vkHandle = VK_NULL_HANDLE;
		vkCreatePipelineLayout( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkPipelineLayoutObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkPipelineLayoutObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyPipelineLayout( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkRenderPassObject CreateRenderPass( const VkRenderPassCreateInfo &CI )
	{
		VkRenderPass vkHandle = VK_NULL_HANDLE;
		vkCreateRenderPass( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkRenderPassObject( this->shared_from_this(), vkHandle );
	}
	void DeleteObject( VkRenderPassObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyRenderPass( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}
	VkFramebufferObject CreateFramebuffer( const VkFramebufferCreateInfo &CI )
	{
		VkFramebuffer vkHandle = VK_NULL_HANDLE;
		vkCreateFramebuffer( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkFramebufferObject( this->shared_from_this(), vkHandle );
	}
	void DeleteObject( VkFramebufferObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyFramebuffer( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkCommandPoolObject CreateCommandPool( const VkCommandPoolCreateInfo &CI )
	{
		VkCommandPool vkHandle = VK_NULL_HANDLE;
		vkCreateCommandPool( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkCommandPoolObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkCommandPoolObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyCommandPool( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkSemaphoreObject CreateSemaphore()
	{
		VkSemaphore vkHandle = VK_NULL_HANDLE;
		VkSemaphoreCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		CI.pNext = nullptr;
		VK_CHECK( vkCreateSemaphore( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkSemaphoreObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkSemaphoreObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroySemaphore( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkFenceObject CreateFence()
	{
		VkFence vkHandle = VK_NULL_HANDLE;
		VkFenceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		CI.pNext = nullptr;
		CI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK( vkCreateFence( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		assert( vkHandle != VK_NULL_HANDLE );
		return VkFenceObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkFenceObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyFence( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	std::shared_ptr<VkDeviceMemoryObject> CreateDeviceMemory( const VkMemoryAllocateInfo &CI )
	{
		VkDeviceMemory vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkAllocateMemory( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return std::shared_ptr<VkDeviceMemoryObject>( new VkDeviceMemoryObject( this->shared_from_this(), vkHandle ) );
	}

	void DeleteObject( VkDeviceMemoryObject &&object )
	{
		assert( object.Device->Device == Device );
		vkFreeMemory( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkDescriptorSetLayoutObject CreateDescriptorSetLayout( const VkDescriptorSetLayoutCreateInfo &CI )
	{
		VkDescriptorSetLayout vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateDescriptorSetLayout( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkDescriptorSetLayoutObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkDescriptorSetLayoutObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyDescriptorSetLayout( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	VkDescriptorPoolObject CreateDescriptorPool( const VkDescriptorPoolCreateInfo &CI )
	{
		VkDescriptorPool vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateDescriptorPool( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkDescriptorPoolObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkDescriptorPoolObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroyDescriptorPool( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}
	VkSamplerObject CreateSampler( const VkSamplerCreateInfo &CI )
	{
		VkSampler vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateSampler( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkSamplerObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkSamplerObject &&object )
	{
		assert( object.Device->Device == Device );
		vkDestroySampler( Device, object, PhysicalDevice->Instance->AllocationCallback );
	}

	friend class VkSurfaceObject;
	std::shared_ptr<VkPhysicalDeviceObject> PhysicalDevice;
	uint32_t GraphicsQueueIndex = -1;
	VkQueue GraphicsQueue = VK_NULL_HANDLE;

private:
	VkDevice Device;
};

struct VkSurfaceObject
{
	struct GLFWWindowDeleter
	{
		void operator()( GLFWwindow *window )
		{
			glfwDestroyWindow( window );
		}
	};
	VkSurfaceObject( std::shared_ptr<VkInstanceObject> instance, std::shared_ptr<VkDeviceObject> device ) :
	  Instance( std::move( instance ) ), Device( std::move( device ) )
	{
		if ( glfwVulkanSupported() == 0 ) {
			std::cout << "GLFW do not support Vulkan\n";
			exit( -1 );
		}
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		auto win = glfwCreateWindow( 1024, 768, "Vulkan", nullptr, nullptr );
		assert( win );
		Window.reset( win );
		VK_CHECK( glfwCreateWindowSurface( *Instance, Window.get(), Instance->AllocationCallback, &Surface ) );
		if ( Surface == VK_NULL_HANDLE ) {
			std::cout << "Surface is not supported\n";
			exit( -1 );
		}
		VkBool32 support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR( *( Device->PhysicalDevice ), Device->GraphicsQueueIndex, Surface, &support );
		if ( support == VK_FALSE ) {
			std::cout << "This device does not support present feature\n";
			exit( -1 );
		}

		vkGetDeviceQueue( *Device, Device->GraphicsQueueIndex, 0, &PresentQueue );

		//uint32_t featureCount = 0;
		// VkSurfaceCapabilitiesKHR cap;
		// vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *( Device->PhysicalDevice ), Surface, &cap );

		uint32_t count;
		vkGetPhysicalDeviceSurfaceFormatsKHR( *( Device->PhysicalDevice ), Surface, &count, nullptr );
		supportedFormat.resize( count );
		vkGetPhysicalDeviceSurfaceFormatsKHR( *( Device->PhysicalDevice ), Surface, &count, supportedFormat.data() );

		std::cout << "Supported Format: " << count << "\n";
		for ( int i = 0; i < count; i++ ) {
			if ( supportedFormat[ i ].format == VK_FORMAT_B8G8R8A8_UNORM && supportedFormat[ i ].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
				std::cout << "Format Support";
			}
			std::cout << supportedFormat[ i ].format << " " << supportedFormat[ i ].colorSpace << std::endl;
		}

		vkGetPhysicalDeviceSurfacePresentModesKHR( *( Device->PhysicalDevice ), Surface, &count, nullptr );
		supportedPresentMode.resize( count );
		vkGetPhysicalDeviceSurfacePresentModesKHR( *( Device->PhysicalDevice ), Surface, &count, supportedPresentMode.data() );

	}
	~VkSurfaceObject()
	{
		Release();
	}
	VkSurfaceObject( const VkSurfaceObject & ) = delete;
	VkSurfaceObject &operator=( const VkSurfaceObject & ) = delete;
	VkSurfaceObject( VkSurfaceObject &&rhs ) noexcept :
	  Instance( std::move( rhs.Instance ) ), Surface( rhs.Surface ), Window( std::move( rhs.Window ) ), Device( std::move( rhs.Device ) )
	{
		rhs.Surface = VK_NULL_HANDLE;
	}
	VkSurfaceObject &operator=( VkSurfaceObject &&rhs ) noexcept
	{
		Release();
		Window = std::move( rhs.Window );
		Instance = std::move( rhs.Instance );
		Device = std::move( rhs.Device );
		return *this;
	}

	operator VkSurfaceKHR()
	{
		return Surface;
	}
	operator VkSurfaceKHR() const
	{
		return Surface;
	}

private:
	friend class VkSwapchainObject;
	void Release()
	{
		vkDestroySurfaceKHR( *Instance, Surface, Instance->AllocationCallback );
		Device = nullptr;
		Instance = nullptr;
		Window = nullptr;
	}

	VkSurfaceKHR Surface;
	VkQueue PresentQueue = VK_NULL_HANDLE;
	std::shared_ptr<VkInstanceObject> Instance;
	std::shared_ptr<VkDeviceObject> Device;
	std::vector<VkSurfaceFormatKHR> supportedFormat;
	std::vector<VkPresentModeKHR> supportedPresentMode;

public:
	std::unique_ptr<GLFWwindow, GLFWWindowDeleter> Window;
};

struct VkContext
{
	std::shared_ptr<VkDeviceObject> Device;
	std::shared_ptr<VkSwapchainObject> Swapchain;
	VkCommandPoolObject PersistentCommandPool;
	std::vector<VkCommandBuffer> CmdBuffers;

	VkCommandPoolObject TransientCommandPool;

	VkDescriptorPoolObject DescriptorPool;
	std::vector<VkDescriptorSet> DescriptorSets;

	std::vector<VkFramebufferObject> Framebuffers;
	VkImageObject DepthImage;
	VkImageViewObject DepthImageView;
	VkImageObject ColorImage;
	VkImageViewObject ColorImageView;
	VkSampleCountFlagBits NumSamples = VK_SAMPLE_COUNT_1_BIT;

	VkExtent2D RenderArea;

	VkRenderPassObject RenderPass;
	VkPipelineObject Pipeline;
	VkPipelineLayoutObject PipelineLayout;
	VkBufferObject VertexBuffer;
	VkBufferObject IndexBuffer;
	uint32_t VertexCount = 0;

	VkDeviceMemoryObject StagingBuffer;

	static constexpr int flightImagesCount = 3;
	std::vector<VkSemaphoreObject> imageAvaliableSemaphores;
	std::vector<VkSemaphoreObject> renderFinishedSemaphores;
	std::vector<VkFenceObject> fences;
	std::vector<VkFence> f;
	uint32_t CurrentFrameIndex = 0;
	uint32_t ImageIndex = 0;

private:
	VkCommandBuffer BeginOneTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			TransientCommandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		};

		VkCommandBuffer TransferCmdBuffer;
		vkAllocateCommandBuffers( *Device, &allocInfo, &TransferCmdBuffer );

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK( vkBeginCommandBuffer( TransferCmdBuffer, &beginInfo ) );
		return TransferCmdBuffer;
	}

	void EndOneTimeCommands( VkCommandBuffer cmdBuffer )
	{
		vkEndCommandBuffer( cmdBuffer );
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkQueueSubmit( Device->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle( Device->GraphicsQueue );
		vkFreeCommandBuffers( *Device, TransientCommandPool, 1, &cmdBuffer );
	}

public:
	VkContext( std::shared_ptr<VkDeviceObject> device, std::shared_ptr<VkSwapchainObject> swapchain ) :
	  Device( std::move( device ) ), Swapchain( std::move( swapchain ) )
	{
		// Create CommandPool
		VkCommandPoolCreateInfo cpCI = {};
		cpCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cpCI.pNext = nullptr;
		cpCI.flags = 0;	 // Important
		PersistentCommandPool = Device->CreateCommandPool( cpCI );

		cpCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		TransientCommandPool = Device->CreateCommandPool( cpCI );
		//
		for ( int i = 0; i < flightImagesCount; i++ ) {
			imageAvaliableSemaphores.push_back( Device->CreateSemaphore() );
			renderFinishedSemaphores.push_back( Device->CreateSemaphore() );
			fences.push_back( Device->CreateFence() );
		}
		f = std::vector<VkFence>( fences.begin(), fences.end() );

		// Color Attachment
		NumSamples = Device->PhysicalDevice->GetMaxUsableSampleCount();
		const VkImageCreateInfo colorImageCI = {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			Swapchain->SurfaceFormat.format,
			{ Swapchain->Size.width,
			  Swapchain->Size.height,
			  1 },
			1,
			1,
			NumSamples,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};
		ColorImage = Device->CreateImage( colorImageCI );
		std::cout << "Sample Num: " << NumSamples << std::endl;
		ColorImageView = ColorImage.CreateImageView( Swapchain->SurfaceFormat.format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT );
		TransitionImageLayout( ColorImage, Swapchain->SurfaceFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1 );

		// ImageDepth
		const VkFormat depthImageFormat = Device->PhysicalDevice->FindDepthFormat();
		const VkImageCreateInfo depthImageCI = {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			depthImageFormat,
			{ Swapchain->Size.width,
			  Swapchain->Size.height,
			  1 },
			1,
			1,
			NumSamples,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		DepthImage = Device->CreateImage( depthImageCI );
		DepthImageView = DepthImage.CreateImageView( depthImageFormat, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT );

		TransitionImageLayout( DepthImage, depthImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1 );

		//
	}

	void CopyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size )
	{
		auto cmdBuffer = BeginOneTimeCommands();
		VkBufferCopy copyRegion = {
			0,
			0,
			size
		};
		vkCmdCopyBuffer( cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion );
		EndOneTimeCommands( cmdBuffer );
	}

	void GenerateMipmaps( VkImage image, VkExtent3D extent, uint32_t mipLevels )
	{
		auto cmdBuf = BeginOneTimeCommands();

		VkImageMemoryBarrier barrier = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_TRANSFER_WRITE_BIT,		   /// TODO
			VK_ACCESS_TRANSFER_READ_BIT,		   /// TODO
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  /// TODO
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  /// TODO
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT,
			  0,
			  1,
			  0,
			  1 }
		};

		int w = extent.width, h = extent.height, d = extent.depth;

		for ( uint32_t i = 1; i < mipLevels; i++ ) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier( cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
								  VK_PIPELINE_STAGE_TRANSFER_BIT,
								  0,
								  0, nullptr,
								  0, nullptr,
								  1, &barrier );

			const VkImageBlit blit = {
				{ VK_IMAGE_ASPECT_COLOR_BIT,
				  i - 1,
				  0,
				  1 },	// srcSubresource
				{
				  { 0, 0, 0 },
				  { w, h, d } },  // srcOffsets
				{
				  VK_IMAGE_ASPECT_COLOR_BIT,
				  i,
				  0,
				  1 },
				{ { 0, 0, 0 },
				  { w > 1 ? w / 2 : 1, h > 1 ? h / 2 : 1, d > 1 ? d / 2 : 1 } }
			};
			vkCmdBlitImage( cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR );

			// After bliting, change the layout as shader-readable layout
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier( cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
								  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
								  0, nullptr, 0, nullptr, 1, &barrier );

			if ( w > 1 ) w /= 2;
			if ( h > 1 ) h /= 2;
			if ( d > 1 ) d /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier( cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
							  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							  0, nullptr, 0, nullptr, 1, &barrier );

		EndOneTimeCommands( cmdBuf );
	}

	void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels )
	{
		auto cmdBuffer = BeginOneTimeCommands();

		VkPipelineStageFlags srcStage, dstStage;

		VkImageMemoryBarrier barrier = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			0,
			oldLayout,
			newLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT,
			  0,
			  mipLevels,
			  0,
			  1 }
		};
		if ( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if ( Device->PhysicalDevice->hasDepthStencil( format ) ) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}

		if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		} else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
									VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		} else {
			std::cout << "unsupported layout transition\n";
			exit( -1 );
		}

		vkCmdPipelineBarrier( cmdBuffer,
							  srcStage,
							  dstStage, 0,
							  0, nullptr,
							  0, nullptr,
							  1, &barrier );
		EndOneTimeCommands( cmdBuffer );
	}

	void CopyImage( VkBuffer srcBuffer, VkImage dstImage, VkExtent3D extent )
	{
		auto cmdBuffer = BeginOneTimeCommands();
		VkBufferImageCopy region = {
			0,
			0,
			0,
			{ VK_IMAGE_ASPECT_COLOR_BIT,
			  0,
			  0,
			  1 },
			{ 0, 0, 0 },
			extent
		};
		vkCmdCopyBufferToImage( cmdBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
		EndOneTimeCommands( cmdBuffer );
	}

	void CreateCommandList()
	{
		std::vector<VkCommandBuffer> cmdBuffers( Framebuffers.size() );

		// Initizalize vertex data

		VkCommandBufferAllocateInfo cbAllocInfo = {};
		cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbAllocInfo.pNext = nullptr;
		cbAllocInfo.commandPool = PersistentCommandPool;
		cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbAllocInfo.commandBufferCount = cmdBuffers.size();
		VK_CHECK( vkAllocateCommandBuffers( *Device, &cbAllocInfo, cmdBuffers.data() ) );

		for ( size_t i = 0; i < cmdBuffers.size(); i++ ) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr;
			VK_CHECK( vkBeginCommandBuffer( cmdBuffers[ i ], &beginInfo ) );

			const VkClearValue clearValues[] = {
				{
				  { 0.f, 0.f, 0.f, 1.f },
				},
				{ { 1.f, 0 } },
				{ { 0.f, 0.f, 0.f, 1.f } }
			};

			const VkRenderPassBeginInfo renderPassBeginInfo = {
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				nullptr,
				RenderPass,
				Framebuffers[ i ],
				{ // VkRect2D
				  { 0, 0 },
				  RenderArea },
				sizeof( clearValues ) / sizeof( clearValues[ 0 ] ),
				clearValues
			};

			vkCmdBeginRenderPass( cmdBuffers[ i ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

			vkCmdBindPipeline( cmdBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline );
			VkDeviceSize offset[] = { 0 };
			VkBuffer vertexBuffers[] = { VertexBuffer };

			assert( PipelineLayout.Valid() );
			vkCmdBindDescriptorSets( cmdBuffers[ i ],
									 VK_PIPELINE_BIND_POINT_GRAPHICS,
									 PipelineLayout,
									 0,
									 1,
									 DescriptorSets.data(),
									 0,
									 nullptr );

			vkCmdBindVertexBuffers( cmdBuffers[ i ], 0, 1, vertexBuffers, offset );
			vkCmdBindIndexBuffer( cmdBuffers[ i ], IndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

			//vkCmdDraw( cmdBuffers[ i ], 3, 1, 0, 0 );
			vkCmdDrawIndexed( cmdBuffers[ i ], VertexCount, 1, 0, 0, 0 );

			vkCmdEndRenderPass( cmdBuffers[ i ] );

			VK_CHECK( vkEndCommandBuffer( cmdBuffers[ i ] ) );
		}
		CmdBuffers = std::move( cmdBuffers );
	}

	void SubmitCommandList()
	{
		VK_CHECK( vkWaitForFences( *Device, 1, f.data() + CurrentFrameIndex, VK_TRUE, std::numeric_limits<uint64_t>::max() ) );
		VK_CHECK( vkResetFences( *Device, 1, f.data() + CurrentFrameIndex ) );
		uint32_t imageIndex;
		auto res = vkAcquireNextImageKHR( *Device, *( this->Swapchain ), std::numeric_limits<uint64_t>::max(), imageAvaliableSemaphores[ CurrentFrameIndex ], VK_NULL_HANDLE, &imageIndex );
		VkSemaphore waitSemaphores[] = { imageAvaliableSemaphores[ CurrentFrameIndex ] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[ CurrentFrameIndex ] };
		const VkSubmitInfo submitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1,
			waitSemaphores,
			waitStages,
			1,
			&( this->CmdBuffers )[ imageIndex ],
			1,
			signalSemaphores
		};
		VK_CHECK( vkQueueSubmit( Device->GraphicsQueue, 1, &submitInfo, fences[ CurrentFrameIndex ] ) );
		const VkSemaphore presentWatiSemaphore[] = { renderFinishedSemaphores[ CurrentFrameIndex ] };
		const VkSwapchainKHR swapchains[] = { *this->Swapchain };
		VkPresentInfoKHR presentInfo = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1,
			presentWatiSemaphore,
			1,
			swapchains,
			&imageIndex,
			nullptr
		};
		res = vkQueuePresentKHR( Device->GraphicsQueue, &presentInfo );
		CurrentFrameIndex = ( CurrentFrameIndex + 1 ) % flightImagesCount;
	}
	void CreateFramebuffers()
	{
		assert( Swapchain );
		assert( Device );
		assert( RenderPass.Valid() );
		std::vector<VkFramebufferObject> framebuffers( Swapchain->SwapchainImageViews.size() );
		framebuffers.resize( Swapchain->SwapchainImageViews.size() );  // dependent by swapchain
		for ( int i = 0; i < Swapchain->SwapchainImageViews.size(); i++ ) {
			const VkImageView attachments[] = {
				ColorImageView,
				DepthImageView,
				Swapchain->SwapchainImageViews[ i ]
			};	// corresponding to the attachments defined in the renderpass instance needed by the framebuffer
			const VkFramebufferCreateInfo CI = {
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				nullptr,
				0,
				RenderPass,
				sizeof( attachments ) / sizeof( attachments[ 0 ] ),
				attachments,
				Swapchain->Size.width,
				Swapchain->Size.height,
				1,
			};
			framebuffers[ i ] = Device->CreateFramebuffer( CI );
		}
		Framebuffers = std::move( framebuffers );
	}
};

// VkObject definition

template <typename T>
void VkObject<T>::Release()
{
	if ( Device && Object != VK_NULL_HANDLE ) {
		Device->DeleteObject( std::move( *this ) );
		Object = VK_NULL_HANDLE;
		Device.reset();
	}
}

template <typename T>
VkObject<T>::VkObject( VkObject &&rhs ) noexcept :
  Device( std::move( rhs.Device ) ), Object( rhs.Object )
{
	rhs.Object = VK_NULL_HANDLE;
}

template <typename T>
VkObject<T> &VkObject<T>::operator=( VkObject &&rhs ) noexcept
{
	Release();
	Device = std::move( rhs.Device );
	Object = rhs.Object;
	rhs.Object = VK_NULL_HANDLE;
	return *this;
}

VkSwapchainObject::VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, VkSwapchainKHR vkHandle ) :
  BaseType( std::move( device ), vkHandle )
{
	uint32_t actualImageCount = 0;
	vkGetSwapchainImagesKHR( *Device, Object, &actualImageCount, nullptr );
	SwapchainImages.resize( actualImageCount );
	vkGetSwapchainImagesKHR( *Device, Object, &actualImageCount, SwapchainImages.data() );
}

VkSwapchainObject::VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, std::shared_ptr<VkSurfaceObject> surface )
{
	assert( surface );
	Device = std::move( device );
	Surface = std::move( surface );
	VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *( Device->PhysicalDevice ), *Surface, &cap ) );
	SurfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	int width, height;
	glfwGetFramebufferSize( Surface->Window.get(), &width, &height );
	Size = { uint32_t( width ), uint32_t( height ) };

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = nullptr;
	swapchainCI.oldSwapchain = VK_NULL_HANDLE;
	swapchainCI.surface = *Surface;
	swapchainCI.minImageCount = 3;
	swapchainCI.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	swapchainCI.imageExtent = Size;
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.imageFormat = SurfaceFormat.format;
	swapchainCI.imageColorSpace = SurfaceFormat.colorSpace;
	swapchainCI.minImageCount = cap.minImageCount + 1;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.preTransform = cap.currentTransform;
	swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCI.clipped = VK_TRUE;

	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.pQueueFamilyIndices = nullptr;
	swapchainCI.queueFamilyIndexCount = 0;

	VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
	assert( Device );
	VK_CHECK( vkCreateSwapchainKHR( *Device, &swapchainCI, Device->PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
	Object = std::move( vkHandle );

	// Get images of swapchain
	uint32_t actualImageCount = 0;
	vkGetSwapchainImagesKHR( *Device, vkHandle, &actualImageCount, nullptr );
	SwapchainImages.resize( actualImageCount );
	vkGetSwapchainImagesKHR( *Device, vkHandle, &actualImageCount, SwapchainImages.data() );

	// Create image views of images of swapchain
	SwapchainImageViews.resize( actualImageCount );
	for ( int i = 0; i < actualImageCount; i++ ) {
		VkImageViewCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		CI.pNext = nullptr;
		CI.image = SwapchainImages[ i ];
		CI.format = SurfaceFormat.format;
		CI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		CI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		CI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		CI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		CI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		CI.subresourceRange.baseMipLevel = 0;
		CI.subresourceRange.layerCount = 1;
		CI.subresourceRange.levelCount = 1;
		CI.subresourceRange.baseArrayLayer = 0;
		CI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageViewObject imageViewObject;
		SwapchainImageViews[ i ] = Device->CreateImageView( CI );
	}

	// set callback
	glfwSetFramebufferSizeCallback( Surface->Window.get(), glfwWindowResizeCallback );
}

// VkDeviceObject definition
//
void VkBufferObject::SetData( const void *data, size_t size )
{
	assert( data );
	assert( size <= Desc.size );
	void *pData = nullptr;
	assert( Device );
	vkMapMemory( *Device, *Memory, 0, VK_WHOLE_SIZE, 0, &pData );
	assert( pData );
	memcpy( pData, data, size );
	vkUnmapMemory( *Device, *Memory );
}

VkImageViewObject VkImageObject::CreateImageView( VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags )
{
	assert( Object != VK_NULL_HANDLE );
	const VkImageViewCreateInfo viewInfo = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		Object,
		viewType,
		format,
		{ VK_COMPONENT_SWIZZLE_IDENTITY,
		  VK_COMPONENT_SWIZZLE_IDENTITY,
		  VK_COMPONENT_SWIZZLE_IDENTITY,
		  VK_COMPONENT_SWIZZLE_IDENTITY },
		{ aspectFlags,
		  0,
		  Desc.mipLevels,
		  0,
		  1 }
	};
	return Device->CreateImageView( viewInfo );
}
