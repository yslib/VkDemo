
#include <VKContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
using namespace std;

int main(){
  
  static const auto vertices = vector<float>{
    0, 0, 0,
    1, 0, 0,
    1, 0, 1,
    0, 0, 1,
    0, 1, 0,
    1, 1, 0,
    1, 1, 1,
    0, 1, 1
  };

  static const auto indices = vector<uint32_t>{
    0, 1, 5, 0, 5, 4,
    1, 6, 5, 1, 2, 6,
    7, 6, 2, 7, 2, 3,
    4, 7, 3, 4, 3, 0,
    5, 7, 4, 5, 6, 7,
    3, 1, 0, 3, 2, 1
  };

	auto instance = std::make_shared<VkInstanceObject>();
	auto physicalDevice = std::make_shared<VkPhysicalDeviceObject>( instance );
	auto device = std::make_shared<VkDeviceObject>( physicalDevice );
	auto surface = std::make_shared<VkSurfaceObject>( instance, device );
	auto swapchain = std::make_shared<VkSwapchainObject>( device, surface );
	auto context = std::make_shared<VkContext>( device, swapchain );

	VkBufferCreateInfo stagingBufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,																	   //flags
		vertices.size() * sizeof( float ),									   //size
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  //usage
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	VkBufferObject vertexStagingBuffer = device->CreateBuffer( stagingBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	vertexStagingBuffer.SetData( vertices.data(), stagingBufferCreateInfo.size );

	VkBufferCreateInfo vertexBufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,																	   //flags
		vertices.size() * sizeof( float ),									   //size
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  //usage
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	VkBufferObject vertexBuffer = device->CreateBuffer( vertexBufferCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	// Create Index Buffer
	VkDeviceSize indexBufferSize = sizeof( indices[ 0 ] ) * indices.size();
	VkBufferCreateInfo indexBufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		indexBufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	context->IndexBuffer = device->CreateBuffer( indexBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	( context->IndexBuffer ).SetData( indices.data(), indexBufferSize ); // why index buffer is a stagingbuffer
	context->CopyBuffer( vertexStagingBuffer, vertexBuffer, vertices.size() * sizeof( float ) );
	context->VertexCount = indices.size();

	std::cout << "Vertex Count: " << context->VertexCount << std::endl;

  return 0;
}
