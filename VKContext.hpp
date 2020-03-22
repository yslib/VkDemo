#include <glm/fwd.hpp>
#include <memory>
#include <vector>
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
		glfwInit();

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
		if ( enableValidationLayers ) {
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
			VkPhysicalDeviceFeatures feat;
			auto d = dev[ i ];
			vkGetPhysicalDeviceProperties( d, &prop );
			vkGetPhysicalDeviceFeatures( d, &feat );

			//vkGetPhysicalDeviceProperties(d,&prop);
			//vkGetPhysicalDeviceFeatures(d, &feat);

			// We don't do any assumption for device, just choose the first physical device
			PhysicalDevice = d;
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
	VkObject( VkObject &&rhs ) noexcept;
	VkObject &operator=( const VkObject & ) = delete;
	VkObject &operator=( VkObject &&rhs ) noexcept;

	operator VkObjectType() { return Object; }
	~VkObject() { Release(); }

  friend std::ofstream & operator<<( std::ofstream & os, const VkObject & object){
    os<<"[Object: "<<object.Object<<", from Device: "<<object.Device<<"]\n";
    return os;
  }

protected:
	std::shared_ptr<VkDeviceObject> Device;
	friend class VkDeviceObject;
	VkObject( std::shared_ptr<VkDeviceObject> device, VkObjectType vkObject ) :
	  Device( std::move( device ) ), Object( vkObject ) {}
	VkObjectType Object;
};

using VkBufferObject = VkObject<VkBuffer>;
using VkBufferViewObject = VkObject<VkBufferView>;
using VkImageObject = VkObject<VkImage>;
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

struct VkSurfaceObject;

struct VkSwapchainObject : public VkSwapchainObjectBase
{
	using BaseType = VkSwapchainObjectBase;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageViewObject> SwapchainImageViews;
	VkSurfaceFormatKHR SurfaceFormat;
	VkExtent2D Size;
	VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, std::shared_ptr<VkSurfaceObject> surface );

private:
	friend class VkDeviceObject;
	VkSwapchainObject( std::shared_ptr<VkDeviceObject> device, VkSwapchainKHR vkHandle );
};

//using VkSwapchainObject = VkObject<VkSwapchainKHR>;

struct VkDeviceObject : public std::enable_shared_from_this<VkDeviceObject>
{
	VkDeviceObject( std::shared_ptr<VkPhysicalDeviceObject> physicalDevice ) :
	  PhysicalDevice( std::move( physicalDevice ) )
	{
		VkDeviceCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		CI.pNext = nullptr;

		// queue family
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( *PhysicalDevice, &count, nullptr );
		assert( count > 0 );
		std::vector<VkQueueFamilyProperties> que( count );
		vkGetPhysicalDeviceQueueFamilyProperties( *PhysicalDevice, &count, que.data() );

		for ( int i = 0; i < count; i++ ) {
			const auto q = que[ i ];

			if ( q.queueCount > 0 && q.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
				GraphicsQueueIndex = i;
				break;
			}
		}
		assert( GraphicsQueueIndex != -1 );

		VkDeviceQueueCreateInfo queCI = {};
		queCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queCI.pNext = nullptr;
		queCI.queueFamilyIndex = GraphicsQueueIndex;
		queCI.queueCount = 1;
		float pri = 1.0;
		queCI.pQueuePriorities = &pri;
		VkPhysicalDeviceFeatures feat = {};

		CI.pQueueCreateInfos = &queCI;
		CI.queueCreateInfoCount = 1;

		CI.enabledLayerCount = validationLayers.size();
		CI.ppEnabledLayerNames = validationLayers.data();

		CI.ppEnabledExtensionNames = deviceExtensions.data();  // for swap chain
		CI.enabledExtensionCount = deviceExtensions.size();

		CI.pEnabledFeatures = &feat;

		VK_CHECK( vkCreateDevice( *PhysicalDevice, &CI, PhysicalDevice->Instance->AllocationCallback, &Device ) );

		vkGetDeviceQueue( Device, GraphicsQueueIndex, 0, &GraphicsQueue );
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

	VkImage CreateImage( const VkImageCreateInfo &CI )
	{
		VkImage vkHandle = VK_NULL_HANDLE;
		VK_CHECK( vkCreateImage( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle ) );
		return VkImageObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkImageObject &&object )
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
	VkBuffer CreateBuffer( const VkBufferCreateInfo &CI )
	{
		VkBuffer vkHandle = VK_NULL_HANDLE;
		vkCreateBuffer( Device, &CI, PhysicalDevice->Instance->AllocationCallback, &vkHandle );
		return VkBufferObject( this->shared_from_this(), vkHandle );
	}

	void DeleteObject( VkBufferObject &&object )
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
    VkFenceCreateInfo CI={};
    CI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    CI.pNext = nullptr;
    CI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle));
    assert(vkHandle != VK_NULL_HANDLE);
    return VkFenceObject(this->shared_from_this(),vkHandle);
	}

  void DeleteObject(VkFenceObject && object){
		assert( object.Device->Device == Device );
    vkDestroyFence(Device,object,PhysicalDevice->Instance->AllocationCallback);
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
		if ( glfwVulkanSupported() == NULL ) {
			std::cout << "GLFW do not support Vulkan\n";
			exit( -1 );
		}
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		VkResult a;
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

		uint32_t featureCount = 0;
		VkSurfaceCapabilitiesKHR cap;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *( Device->PhysicalDevice ), Surface, &cap );

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
	VkSurfaceCapabilitiesKHR cap;

	Size = { 1024, 768 };
	SurfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *( Device->PhysicalDevice ), *surface, &cap ) );
	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = nullptr;
	swapchainCI.oldSwapchain = VK_NULL_HANDLE;
	swapchainCI.surface = *surface;
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
}

// VkDeviceObject definition
