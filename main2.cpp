#include <bits/stdint-uintn.h>
#include <glm/fwd.hpp>
#include <memory>
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
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
//#include <StandAlone/DirStackFileIncluder.h>
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

VkShaderModule CreateVkShaderModule(VkDevice * device,const std::string & fileName){
  return VK_NULL_HANDLE;
}

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


template<typename T>
struct VkObject;


//using VkBufferObject = VkObject<VkBuffer>;
//


struct VkDeviceObject{
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

  private:
  friend class VkSurfaceObject;
  std::shared_ptr<VkPhysicalDeviceObject> PhysicalDevice;
  VkDevice Device;
  uint32_t GraphicsQueueIndex = -1;
  VkQueue GraphicsQueue = VK_NULL_HANDLE;
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

template<typename VkObjectType>
class VkObject
{
  VkObject()=default;
    void Release(){
      if(Device && Object != VK_NULL_HANDLE){
        Device->DeleteObject(std::move(*this));
        Object = VK_NULL_HANDLE;
        Device.reset();
      }
    }
    VkObject(const VkObject& )=delete;
    VkObject & operator=(const VkObject &)=delete;
    VkObject(VkObject && rhs)noexcept:Device(std::move(rhs.Device)),Object(rhs.Object){
      rhs.Object  = VK_NULL_HANDLE;
    }
    VkObject & operator=(VkObject && rhs)noexcept{
      Release();
      Device = std::move(rhs.Device);
      Object = rhs.Object;
      rhs.Object= VK_NULL_HANDLE;
    }
    operator VkObjectType(){
      return Object;
    }

    ~VkObject(){
    }
  private:
    std::shared_ptr<VkDeviceObject> Device;
    friend class VkDeviceObject;
    VkObject(std::shared_ptr<VkDeviceObject> device, VkObjectType vkObject):Device(std::move(device)),Object(vkObject)
    {
    }
   VkObjectType Object;
};

int main(){

  auto instance = std::make_shared<VkInstanceObject>();
  auto physicalDevice = std::make_shared<VkPhysicalDeviceObject>(instance);
  auto device = std::make_shared<VkDeviceObject>(physicalDevice);
  auto surface = std::make_shared<VkSurfaceObject>(instance,device);


  return 0;
}
