#include <glm/fwd.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
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

//
#define VK_CHECK(expr) \
do{\
  VkResult res; \
  if((res = expr)!= VK_SUCCESS) {\
    std::cout<<"VkResult("<<res<<"): "<<#expr<<": Vulkan Assertion Failed:";\
    exit(-1);\
  }\
}while(false);
VkAllocationCallbacks * g_allocation = nullptr;



struct VkInstanceObject
{
  VkInstanceObject(){

    uint32_t extPropCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extPropCount, nullptr);
    std::vector<VkExtensionProperties> props(extPropCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extPropCount, props.data());
    for(int i = 0 ;i<props.size();i++){
      std::cout<<props[i].extensionName<<std::endl;
    }

    glfwInit();
    


    uint32_t extCount=0;
    auto ext = glfwGetRequiredInstanceExtensions(&extCount);

    VkInstanceCreateInfo CI = {};
    CI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CI.pNext = nullptr;
  
    VkApplicationInfo appCI={};
    appCI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appCI.pNext = nullptr;
    appCI.pEngineName = "My Engine";
    appCI.pApplicationName = "My Vulkan";
    appCI.apiVersion = VK_MAKE_VERSION(0, 0, 0);
    appCI.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    appCI.applicationVersion = VK_MAKE_VERSION(0, 0, 0);

    CI.pApplicationInfo = nullptr;
    CI.enabledExtensionCount = extCount;
    CI.ppEnabledExtensionNames = ext;


    CI.enabledLayerCount = validationLayers.size();
    CI.ppEnabledLayerNames = validationLayers.data();

    VK_CHECK(vkCreateInstance(&CI,AllocationCallback,&vkInstance));
  }
  ~VkInstanceObject(){
    vkDestroyInstance(vkInstance,AllocationCallback);
  }
  operator VkInstance(){
    return vkInstance;
  }
  VkAllocationCallbacks *AllocationCallback = nullptr;
  private:
  VkInstance vkInstance;
};

struct VkPhysicalDeviceObject{
  std::shared_ptr<VkInstanceObject> Instance;
  VkPhysicalDeviceObject(std::shared_ptr<VkInstanceObject> instance):Instance(std::move(instance))
  {
    uint32_t count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(*Instance, &count,nullptr));
    using namespace std;
    vector<VkPhysicalDevice> dev(count);
    VK_CHECK(vkEnumeratePhysicalDevices(*Instance, &count,dev.data()));
    std::cout<<"There are "<<count<<" physical device(s)\n";
    for(int i = 0 ; i < count;i++)
    {
      VkPhysicalDeviceProperties prop;
      VkPhysicalDeviceFeatures feat;
      auto d = dev[i];
      vkGetPhysicalDeviceProperties(d,&prop);
      vkGetPhysicalDeviceFeatures(d,&feat);

      //vkGetPhysicalDeviceProperties(d,&prop);
      //vkGetPhysicalDeviceFeatures(d, &feat);

      // We don't do any assumption for device, just choose the first physical device
      PhysicalDevice = d;
      break;
    }
    
  }

  VkPhysicalDeviceObject(const VkPhysicalDeviceObject &) =delete;
  VkPhysicalDeviceObject & operator=(const VkPhysicalDeviceObject &)=delete;
  VkPhysicalDeviceObject(VkPhysicalDeviceObject &&)noexcept = default;
  VkPhysicalDeviceObject & operator=(VkPhysicalDeviceObject &&)noexcept = default;


  operator VkPhysicalDevice(){
    return PhysicalDevice;
  }

  private:
  VkPhysicalDevice PhysicalDevice;
};

struct VkDeviceObject;

template<typename VkObjectType>
struct VkObject
{
  VkObject()=default;
  void Release();
  VkObject(const VkObject& )=delete;
  VkObject(VkObject && rhs)noexcept;
  VkObject & operator=(const VkObject &)=delete;
  VkObject & operator=(VkObject && rhs)noexcept;

  operator VkObjectType(){return Object;}
  ~VkObject(){Release();}
  protected:
   std::shared_ptr<VkDeviceObject> Device;
   friend class VkDeviceObject;
 VkObject(std::shared_ptr<VkDeviceObject> device, VkObjectType vkObject):Device(std::move(device)),Object(vkObject){}
   VkObjectType Object;
};

using VkBufferObject = VkObject<VkBuffer>;
using VkBufferViewObject = VkObject<VkBufferView>;
using VkImageObject= VkObject<VkImage>;
using VkImageViewObject= VkObject<VkImageView>;
using VkSwapchainObjectBase = VkObject<VkSwapchainKHR>;
using VkPipelineObject = VkObject<VkPipeline>;
using VkShaderModuleObject = VkObject<VkShaderModule>;

struct VkSurfaceObject;

struct VkSwapchainObject:public VkSwapchainObjectBase{
  using BaseType = VkSwapchainObjectBase;
  std::vector<VkImage> SwapchainImages;
  std::vector<VkImageViewObject> SwapchainImageViews;
  VkSurfaceFormatKHR SurfaceFormat;
  VkExtent2D Size;
  VkSwapchainObject(std::shared_ptr<VkDeviceObject> device,std::shared_ptr<VkSurfaceObject> surface);
  private:
  friend class VkDeviceObject;
    VkSwapchainObject(std::shared_ptr<VkDeviceObject> device,VkSwapchainKHR vkHandle);
};

//using VkSwapchainObject = VkObject<VkSwapchainKHR>;


struct VkDeviceObject:public std::enable_shared_from_this<VkDeviceObject>{
  VkDeviceObject(std::shared_ptr<VkPhysicalDeviceObject> physicalDevice):PhysicalDevice(std::move(physicalDevice)){
    VkDeviceCreateInfo CI={};
    CI.sType= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CI.pNext = nullptr;

    // queue family
    uint32_t count=0;
    vkGetPhysicalDeviceQueueFamilyProperties(*PhysicalDevice,&count,nullptr);
    assert(count > 0);
    std::vector<VkQueueFamilyProperties> que(count);
    vkGetPhysicalDeviceQueueFamilyProperties(*PhysicalDevice,&count,que.data());

    for(int i = 0;i<count;i++){
      const auto q = que[i];

      if( q.queueCount > 0 &&q.queueFlags & VK_QUEUE_GRAPHICS_BIT){
        GraphicsQueueIndex = i;
        break;
      }
    }
    assert(GraphicsQueueIndex!=-1);

    VkDeviceQueueCreateInfo queCI={};
    queCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queCI.pNext  = nullptr;
    queCI.queueFamilyIndex = GraphicsQueueIndex;
    queCI.queueCount = 1;
    float pri =1.0;
    queCI.pQueuePriorities = &pri;
    VkPhysicalDeviceFeatures feat={};

    CI.pQueueCreateInfos = &queCI;
    CI.queueCreateInfoCount = 1;

    CI.enabledLayerCount = validationLayers.size();
    CI.ppEnabledLayerNames = validationLayers.data();

    CI.ppEnabledExtensionNames = deviceExtensions.data(); // for swap chain
    //CI.enabledExtensionCount = deviceExtensions.size();

    CI.pEnabledFeatures = &feat;


    VK_CHECK(vkCreateDevice(*PhysicalDevice,&CI,PhysicalDevice->Instance->AllocationCallback,&Device)); 

    vkGetDeviceQueue(Device,GraphicsQueueIndex,0,&GraphicsQueue);
    // Get Queue

  }
  ~VkDeviceObject(){
    vkDestroyDevice(Device,PhysicalDevice->Instance->AllocationCallback);
  }

  VkDeviceObject(const VkDeviceObject &)=delete;
  VkDeviceObject & operator=(const VkDeviceObject &) =delete;

  VkDeviceObject(VkDeviceObject && rhs)noexcept:PhysicalDevice(std::move(rhs.PhysicalDevice)),Device(rhs.Device){
    rhs.Device = VK_NULL_HANDLE;
  }

  VkDeviceObject & operator=(VkDeviceObject && rhs)noexcept{
    
    vkDestroyDevice(Device,PhysicalDevice->Instance->AllocationCallback);
    PhysicalDevice  = std::move(rhs.PhysicalDevice);
    Device = rhs.Device;
    rhs.Device = VK_NULL_HANDLE;
    return *this;
  }
  operator VkDevice(){
    return Device;
  }
  operator VkDevice()const{
    return Device;
  }

  VkSwapchainObject CreateSwapchain(const VkSwapchainCreateInfoKHR & CI){
    VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(Device, &CI, PhysicalDevice->Instance->AllocationCallback,&vkHandle)); 
    return VkSwapchainObject(this->shared_from_this(),vkHandle);
  }
  
  void DeleteObject(VkSwapchainObjectBase && object)
  {
    assert(object.Device->Device == Device);
    vkDestroySwapchainKHR(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  VkImage CreateImage(const VkImageCreateInfo & CI){
    VkImage vkHandle = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImage(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle));
    return VkImageObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkImageObject && object){
    assert(object.Device->Device == Device);
    vkDestroyImage(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  VkImageViewObject CreateImageView(const VkImageViewCreateInfo & CI){
    VkImageView vkHandle = VK_NULL_HANDLE;
    vkCreateImageView(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle);
    return VkImageViewObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkImageViewObject && object){
    assert(object.Device->Device == Device);
    vkDestroyImageView(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }
VkBuffer CreateBuffer(const VkBufferCreateInfo & CI){
    VkBuffer vkHandle = VK_NULL_HANDLE;
    vkCreateBuffer(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle);
    return VkBufferObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkBufferObject && object){
    assert(object.Device->Device == Device);
    vkDestroyBuffer(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  VkBufferViewObject CreateBufferView(const VkBufferViewCreateInfo & CI){
    VkBufferView vkHandle = VK_NULL_HANDLE;
    vkCreateBufferView(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle);
    return VkBufferViewObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkBufferViewObject && object){
    assert(object.Device->Device == Device);
    vkDestroyBufferView(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  VkPipelineObject CreatePipeline(const VkGraphicsPipelineCreateInfo & CI){
    VkPipeline vkHandle = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(Device,VK_NULL_HANDLE,1,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle);
    return VkPipelineObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkPipelineObject && object){
    assert(object.Device->Device == Device);
    vkDestroyPipeline(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  VkShaderModuleObject CreateShader(const VkShaderModuleCreateInfo & CI){
    VkShaderModule vkHandle = VK_NULL_HANDLE;
    vkCreateShaderModule(Device,&CI,PhysicalDevice->Instance->AllocationCallback,&vkHandle);
    return VkShaderModuleObject(this->shared_from_this(),vkHandle);
  }

  void DeleteObject(VkShaderModuleObject && object){
    assert(object.Device->Device == Device);
    vkDestroyShaderModule(Device,object,PhysicalDevice->Instance->AllocationCallback);
  }

  

  friend class VkSurfaceObject;
  std::shared_ptr<VkPhysicalDeviceObject> PhysicalDevice;
  uint32_t GraphicsQueueIndex = -1;
  VkQueue GraphicsQueue = VK_NULL_HANDLE;
  private:
  VkDevice Device;
  };



struct VkSurfaceObject{
  struct GLFWWindowDeleter{
    void operator()(GLFWwindow * window){
      glfwDestroyWindow(window);
    }
  };
  VkSurfaceObject(std::shared_ptr<VkInstanceObject> instance,std::shared_ptr<VkDeviceObject> device):Instance(std::move(instance)),Device(std::move(device)){
    if(glfwVulkanSupported() == NULL){
      std::cout<<"GLFW do not support Vulkan\n";
      exit(-1);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    VkResult a;
    auto win = glfwCreateWindow(1024,768,"Vulkan",nullptr,nullptr);
    assert(win);
    Window.reset(win);
    VK_CHECK(glfwCreateWindowSurface(*Instance, Window.get(), Instance->AllocationCallback, &Surface));
    if(Surface == VK_NULL_HANDLE){
      std::cout<<"Surface is not supported\n";
      exit(-1);
    }
    VkBool32 support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(*(Device->PhysicalDevice),Device->GraphicsQueueIndex,Surface,&support);
    if(support == VK_FALSE){
      std::cout<<"This device does not support present feature\n";
      exit(-1);
    }
    vkGetDeviceQueue(*Device,Device->GraphicsQueueIndex,0,&PresentQueue);
  }
  ~VkSurfaceObject(){
    Release();
  }
  VkSurfaceObject(const VkSurfaceObject &)=delete;
  VkSurfaceObject & operator=(const VkSurfaceObject &)=delete;
  VkSurfaceObject(VkSurfaceObject && rhs)noexcept:Instance(std::move(rhs.Instance)),Surface(rhs.Surface),Window(std::move(rhs.Window)),Device(std::move(rhs.Device)){
    rhs.Surface = VK_NULL_HANDLE;
  }
  VkSurfaceObject & operator=(VkSurfaceObject && rhs)noexcept{
    Release();
    Window = std::move(rhs.Window);
    Instance = std::move(rhs.Instance);
    Device = std::move(rhs.Device);
    return *this;
  }

  operator VkSurfaceKHR(){
    return Surface;
  }
  operator VkSurfaceKHR()const{
    return Surface;
  }

  private:
  void Release(){
    vkDestroySurfaceKHR(*Instance,Surface,Instance->AllocationCallback);
    Device = nullptr;
    Instance = nullptr;
    Window = nullptr;
  }
  
  VkSurfaceKHR Surface;
  VkQueue PresentQueue = VK_NULL_HANDLE;
  std::unique_ptr<GLFWwindow,GLFWWindowDeleter> Window;
  std::shared_ptr<VkInstanceObject> Instance;
  std::shared_ptr<VkDeviceObject> Device;
};


// VkObject definition

template<typename T>
void VkObject<T>::Release(){
      if(Device && Object != VK_NULL_HANDLE){
        Device->DeleteObject(std::move(*this));
        Object = VK_NULL_HANDLE;
        Device.reset();
      }
}

template<typename T>
VkObject<T>::VkObject(VkObject && rhs)noexcept:Device(std::move(rhs.Device)),Object(rhs.Object){
      rhs.Object  = VK_NULL_HANDLE;
}

template<typename T>
VkObject<T> & VkObject<T>::operator=(VkObject && rhs)noexcept
{
      Release();
      Device = std::move(rhs.Device);
      Object = rhs.Object;
      rhs.Object= VK_NULL_HANDLE;
}

VkSwapchainObject::VkSwapchainObject(std::shared_ptr<VkDeviceObject> device,VkSwapchainKHR vkHandle):BaseType(std::move(device),vkHandle){
      uint32_t actualImageCount = 0;
      vkGetSwapchainImagesKHR(*Device,Object,&actualImageCount,nullptr);
      SwapchainImages.resize(actualImageCount);
      vkGetSwapchainImagesKHR(*Device,Object,&actualImageCount,SwapchainImages.data());
}

VkSwapchainObject::VkSwapchainObject(std::shared_ptr<VkDeviceObject> device,std::shared_ptr<VkSurfaceObject> surface){
  Device = std::move(device);
  VkSurfaceCapabilitiesKHR cap;

  Size ={1024,768};
  SurfaceFormat = {VK_FORMAT_R8G8B8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*(Device->PhysicalDevice),*surface,&cap));
  VkSwapchainCreateInfoKHR swapchainCI={};
  swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCI.pNext= nullptr;
  swapchainCI.oldSwapchain = VK_NULL_HANDLE;
  swapchainCI.surface = *surface;
  swapchainCI.minImageCount = 3;
  swapchainCI.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
  swapchainCI.imageExtent = Size;

  swapchainCI.imageFormat = SurfaceFormat.format;
  swapchainCI.imageColorSpace = SurfaceFormat.colorSpace;

  swapchainCI.minImageCount = cap.minImageCount + 1;
  swapchainCI.imageArrayLayers = 1;
  swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCI.pQueueFamilyIndices = nullptr;
  swapchainCI.queueFamilyIndexCount =0;
  swapchainCI.preTransform = cap.currentTransform; 
  swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCI.clipped = VK_TRUE;

  VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
  VK_CHECK(vkCreateSwapchainKHR(*Device, &swapchainCI, Device->PhysicalDevice->Instance->AllocationCallback,&vkHandle)); 
  Object = std::move(vkHandle);


   // Get images of swapchain
  uint32_t actualImageCount = 0;
  vkGetSwapchainImagesKHR(*Device, vkHandle,&actualImageCount,nullptr);
  SwapchainImages.resize(actualImageCount);
  vkGetSwapchainImagesKHR(*Device, vkHandle,&actualImageCount,SwapchainImages.data());


  // Create image views of images of swapchain
  SwapchainImageViews.resize(actualImageCount);
  for(int i = 0 ; i < actualImageCount;i++){
    VkImageViewCreateInfo CI={};
    CI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    CI.pNext = nullptr;
    CI.image = SwapchainImages[i];
    CI.format = SurfaceFormat.format;
    CI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    CI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    CI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    CI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    CI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    CI.subresourceRange.baseMipLevel = 0;
    CI.subresourceRange.layerCount =1;
    CI.subresourceRange.baseArrayLayer = 0;
    CI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewObject imageViewObject;
    SwapchainImageViews[i] = Device->CreateImageView(CI);

  }

}
// VkDeviceObject definition

