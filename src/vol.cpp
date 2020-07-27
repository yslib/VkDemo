#include <VKContext.hpp>
#include <vector>
#include <fstream>
#include <vulkan/vulkan_core.h>
using namespace std;

int main()
{
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

	// create vertex buffer
	const VkBufferCreateInfo vsbci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		vertices.size() * sizeof( float ),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	// staging buffer
	auto vertexStageBuffer = device->CreateBuffer( vsbci, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	vertexStageBuffer.SetData( vertices.data(), sizeof( float ) * vertices.size() );

	const VkBufferCreateInfo vbci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
    vsbci.size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	context->VertexBuffer = device->CreateBuffer( vbci, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
  context->CopyBuffer(vertexStageBuffer,context->VertexBuffer,vsbci.size);

	// create index buffer
	const VkBufferCreateInfo isbci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		vertices.size() * sizeof( float ),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	// staging buffer
	auto indexStagingBuffer = device->CreateBuffer( isbci, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	indexStagingBuffer.SetData( indices.data(), sizeof( unsigned int ) * indices.size() );

	const VkBufferCreateInfo ibci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
    isbci.size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

  context->IndexBuffer = device->CreateBuffer(ibci,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  context->CopyBuffer(indexStagingBuffer,context->IndexBuffer,isbci.size);
  context->VertexCount = indices.size();

  // load volume data
  uint32_t width = 160,height = 240,depth = 40;
  const auto total = width * height * depth;
  

  auto file = ifstream("resources/mixfrac160x240x40.raw",ios::binary);
  if(file.is_open() == false){
    cout<<"failed to open volume data file\n";
    exit(-1);
  }
  vector<char> d(total);
  file.read(d.data(),total);

  VkBufferCreateInfo vdci={
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    nullptr,
    0,
    static_cast<uint32_t>(total),
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_SHARING_MODE_EXCLUSIVE,
    0,
    nullptr
  };

  auto volumeStagingBuffer = device->CreateBuffer(vdci,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  volumeStagingBuffer.SetData(d.data(),total);

  // Create image object
  const VkImageCreateInfo ici={
    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    nullptr,
    0,
    VK_IMAGE_TYPE_3D,
    VK_FORMAT_R8_UNORM,
    {width,height,depth},
    1,1,
    VK_SAMPLE_COUNT_1_BIT,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_SHARING_MODE_EXCLUSIVE,
    0,
    nullptr,
    VK_IMAGE_LAYOUT_UNDEFINED
  };

  auto volumeImage = device->CreateImage(ici);
  auto volumeImageView = volumeImage.CreateImageView(VK_FORMAT_R8_UNORM,VK_IMAGE_VIEW_TYPE_3D,VK_IMAGE_ASPECT_COLOR_BIT);




	return 0;
}
