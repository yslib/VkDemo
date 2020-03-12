//#include <StandAlone/DirStackFileIncluder.h>
#include "VKContext.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
#include "SPIRVConverter.h"

using namespace std;
int main(){

  auto instance = std::make_shared<VkInstanceObject>();
  auto physicalDevice = std::make_shared<VkPhysicalDeviceObject>(instance);
  auto device = std::make_shared<VkDeviceObject>(physicalDevice);
  auto surface = std::make_shared<VkSurfaceObject>(instance,device);
  auto swapchain = std::make_shared<VkSwapchainObject>(device,surface);
  //
  //
  // SPIRVConverter test
  SPIRVConverter cvt;
  auto vShader = cvt.GLSL2SPRIV("test.vert");
  auto fShader = cvt.GLSL2SPRIV("test.frag");

  VkShaderModuleCreateInfo vsCI = {};
  vsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vsCI.pNext = nullptr;
  vsCI.codeSize = vShader.size();
  vsCI.pCode = vShader.data();

  auto vs = device->CreateShader(vsCI);

  VkShaderModuleCreateInfo fsCI = {};
  fsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fsCI.pNext = nullptr;
  fsCI.codeSize = fShader.size();
  fsCI.pCode = fShader.data();

  auto fs = device->CreateShader(fsCI);

  VkPipelineShaderStageCreateInfo vssCI = {};
  vssCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vssCI.pNext = nullptr;
  vssCI.pSpecializationInfo = nullptr;
  vssCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vssCI.module = vs;
  vssCI.pName = "main";

  VkPipelineShaderStageCreateInfo fssCI = {};
  fssCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fssCI.pNext = nullptr;
  fssCI.pSpecializationInfo = nullptr;
  fssCI.flags = VK_SHADER_STAGE_FRAGMENT_BIT;
  fssCI.pName = "main";
  fssCI.module = fs;

  
   VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2]={vssCI,fssCI};
   VkPipelineVertexInputStateCreateInfo vertCreateInfo={};
   VkPipelineInputAssemblyStateCreateInfo asmCreateInfo={};
   asmCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   asmCreateInfo.pNext = nullptr;
   asmCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   asmCreateInfo.primitiveRestartEnable = VK_TRUE;




  return 0;
}
