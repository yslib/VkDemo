//#include <StandAlone/DirStackFileIncluder.h>
#include "VKContext.hpp"
#include <GLFW/glfw3.h>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <limits>
#include <memory>
#include <regex>
#include <type_traits>
#include <utility>
#include <vulkan/vulkan_core.h>
#include <unordered_map>
#include "SPIRVConverter.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace std;

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex
{
	glm::vec3 Pos;
	glm::vec3 Color;
	glm::vec2 texCoord;
	bool operator==( const Vertex &other ) const
	{
		return Pos == other.Pos && Color == other.Color && texCoord == other.texCoord;
	}
};
namespace std
{
template <>
struct hash<Vertex>
{
	size_t operator()( Vertex const &vertex ) const
	{
		return ( ( hash<glm::vec3>()( vertex.Pos ) ^ ( hash<glm::vec3>()( vertex.Color ) << 1 ) ) >> 1 ) ^ ( hash<glm::vec2>()( vertex.texCoord ) << 1 );
	}
};

}  // namespace std
glm::mat4 UpdateModelMatrix()
{
	UniformBufferObject ubo = {};
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float,
									   std::chrono::seconds::period>( currentTime - startTime )
				   .count();

	return glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ),
						glm::vec3( 0.0f, 0.0f, 1.0f ) );
}

std::pair<std::vector<Vertex>, std::vector<uint32_t>> LoadObjFile( const std::string &fileName )
{
	tinyobj::attrib_t attrib;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	std::string warn;

	if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, fileName.c_str(), nullptr, true, true ) ) {
		throw std::runtime_error( err );
	}
	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for ( const auto &shape : shapes ) {
		for ( const auto &index : shape.mesh.indices ) {
			Vertex vertex = {};
			vertex.Pos = {
				attrib.vertices[ 3 * index.vertex_index + 0 ],
				attrib.vertices[ 3 * index.vertex_index + 1 ],
				attrib.vertices[ 3 * index.vertex_index + 2 ]
			};

			vertex.texCoord = {
				attrib.texcoords[ 2 * index.texcoord_index + 0 ],
				1.0f - attrib.texcoords[ 2 * index.texcoord_index + 1 ]
			};

			vertex.Color = { 1.0f, 1.0f, 1.0f };

			if ( uniqueVertices.count( vertex ) == 0 ) {
				uniqueVertices[ vertex ] = static_cast<uint32_t>( vertices.size() );
				vertices.push_back( vertex );
			}
			indices.push_back( uniqueVertices[ vertex ] );
		}
	}
	return std::make_pair( std::move( vertices ), std::move( indices ) );
}

vector<char> OpenSprivFromFile( const std::string &fileName )
{
	ifstream fileIn( fileName, std::ios::binary | std::ios::ate );
	if ( fileIn.is_open() == false ) {
		cout << "Failed to open SPIRV: " << fileName << std::endl;
		exit( -1 );
	}
	vector<char> code( fileIn.tellg() );
	fileIn.seekg( 0, std::ios::beg );
	fileIn.read( code.data(), code.size() );
	return code;
}

std::pair<VkBufferObject, VkExtent3D> CreateTextureStagingBuffer( VkDeviceObject *device, const std::string &fileName ,uint32_t  * mipLevel)
{
	int width, height, channel;
	auto p = stbi_load( fileName.c_str(), &width, &height, &channel, STBI_rgb_alpha );

	VkDeviceSize size = width * height * 4;
	if ( !p ) {
		std::cout << "Failed to load image" << std::endl;
		exit( -1 );
	}
	VkBufferCreateInfo CI = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	auto bufferObject = device->CreateBuffer( CI, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
	bufferObject.SetData( p, size );
	stbi_image_free( p );

  if(mipLevel)
  {
    *mipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  }
	return std::make_pair( std::move( bufferObject ), VkExtent3D{ uint32_t( width ), uint32_t( height ), 1 } );
}

int main()
{
	auto v = LoadObjFile( "resources/chalet.obj" );

	const auto vertices = std::move( v.first );
	const auto indices = std::move( v.second );

	// Initialize VK
	auto instance = std::make_shared<VkInstanceObject>();
	auto physicalDevice = std::make_shared<VkPhysicalDeviceObject>( instance );
	auto device = std::make_shared<VkDeviceObject>( physicalDevice );
	auto surface = std::make_shared<VkSurfaceObject>( instance, device );
	auto swapchain = std::make_shared<VkSwapchainObject>( device, surface );
	auto context = std::make_shared<VkContext>( device, swapchain );

	bool framebufferResize = false;
	FramebufferResizeEventCallback callback = [&framebufferResize]( void *userData, int width, int height ) {
		framebufferResize = true;
	};
	context->Swapchain->SetResizeCallback( callback );

	// Create Vertex Data
	VkBufferCreateInfo stagingBufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,																	   //flags
		vertices.size() * sizeof( Vertex ),									   //size
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
		vertices.size() * sizeof( Vertex ),									   //size
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
	( context->IndexBuffer ).SetData( indices.data(), indexBufferSize );
	context->CopyBuffer( vertexStagingBuffer, vertexBuffer, vertices.size() * sizeof( Vertex ) );
	context->VertexCount = indices.size();
  std::cout<<"Vertex Count: "<<context->VertexCount<<std::endl;


	// Create Texture Staging Buffer
  uint32_t mipLevels = 1;
	auto tex = CreateTextureStagingBuffer( device.get(), "resources/chalet.jpg" ,&mipLevels);

  std::cout<<"Mipmap Levels: "<<mipLevels<<std::endl;
	assert( tex.first.Valid() );
	VkImageCreateInfo imageCI = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_R8G8B8A8_UNORM,
		tex.second,
		mipLevels,	// mipLevel
		1,	// arraylayers
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED  // why
	};

	auto imageObject = device->CreateImage( imageCI );
	auto imageViewObject = imageObject.CreateImageView( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

	context->TransitionImageLayout( imageObject, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
	context->CopyImage( tex.first, imageObject, tex.second );
  context->GenerateMipmaps(imageObject,imageCI.extent,mipLevels);
	//context->TransitionImageLayout( imageObject, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,mipLevels);

	const VkSamplerCreateInfo samplerCI = {
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, // mipMapMode
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		0.f, // mipLodBias
		VK_TRUE,
		16,
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		0,  //minlod
		float(mipLevels),  //maxlod
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_FALSE
	};

	auto sampler = device->CreateSampler( samplerCI );

	VkDeviceSize uboSize = sizeof( UniformBufferObject );
	VkBufferCreateInfo uboBufCI = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		uboSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	vector<VkBufferObject> uboBuffers;
	for ( int i = 0; i < context->Swapchain->SwapchainImageViews.size(); i++ ) {
		uboBuffers.push_back( device->CreateBuffer( uboBufCI, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) );
	}

	// Configuration Pipeline

	auto vShader = OpenSprivFromFile( "vert.spv" );
	auto fShader = OpenSprivFromFile( "frag.spv" );

	VkShaderModuleCreateInfo vsCI = {};
	vsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vsCI.pNext = nullptr;
	vsCI.codeSize = vShader.size();
	vsCI.pCode = (uint32_t *)vShader.data();

	auto vs = device->CreateShader( vsCI );

	VkShaderModuleCreateInfo fsCI = {};
	fsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fsCI.pNext = nullptr;
	fsCI.codeSize = fShader.size();
	fsCI.pCode = (uint32_t *)fShader.data();

	auto fs = device->CreateShader( fsCI );

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
	fssCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fssCI.pName = "main";
	fssCI.module = fs;

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[ 2 ] = { vssCI, fssCI };

	VkVertexInputBindingDescription inputBindingDesc[] = {
		{ 0,				 //binding
		  sizeof( Vertex ),	 // stride
		  VK_VERTEX_INPUT_RATE_VERTEX }
	};
	VkVertexInputAttributeDescription inputAttribDesc[] = {
		{
		  0,						   // location
		  0,						   // binding
		  VK_FORMAT_R32G32B32_SFLOAT,  // format2
		  offsetof( Vertex, Pos )	   // offset
		},
		{ 1,
		  0,
		  VK_FORMAT_R32G32B32_SFLOAT,
		  offsetof( Vertex, Color ) },
		{ 2,
		  0,
		  VK_FORMAT_R32G32_SFLOAT,
		  offsetof( Vertex, texCoord ) }
	};

	UniformBufferObject ubo = {};

	// descriptor layout
	VkDescriptorSetLayoutBinding bindings[] = {
		// uniform binding
		{
		  0,								  // binding
		  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // type
		  1,								  //count
		  VK_SHADER_STAGE_VERTEX_BIT,		  // stageFlags
		  nullptr							  //immutable sampler
		},
		// sampler binding
		{
		  1,
		  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		  1,
		  VK_SHADER_STAGE_FRAGMENT_BIT,
		  nullptr }
	};

	VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
	descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCI.pNext = nullptr;
	descSetLayoutCI.bindingCount = 2;
	descSetLayoutCI.pBindings = bindings;

	auto setlayout = device->CreateDescriptorSetLayout( descSetLayoutCI );

	// Configuration Descriptor Pool
	const uint32_t swapchainImageCount = context->Swapchain->SwapchainImageViews.size();

	const VkDescriptorPoolSize poolSize[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		  swapchainImageCount },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		  swapchainImageCount }
	};
	VkDescriptorPoolCreateInfo dpCI = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		swapchainImageCount,
		2,
		poolSize
	};

	context->DescriptorPool = device->CreateDescriptorPool( dpCI );

	vector<VkDescriptorSetLayout> layouts( swapchainImageCount, setlayout );
	VkDescriptorSetAllocateInfo dsAllocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		context->DescriptorPool,
		swapchainImageCount,
		layouts.data()
	};

	vector<VkDescriptorSet> descriptorSets( swapchainImageCount );
	VK_CHECK( vkAllocateDescriptorSets( *device, &dsAllocInfo, descriptorSets.data() ) );

	// binding resources to descriptor set
	for ( int i = 0; i < swapchainImageCount; i++ ) {
		const VkDescriptorBufferInfo bufferInfo[] = {
			{
			  uboBuffers[ i ],
			  0,
			  sizeof( UniformBufferObject )	 // equal to VK_WHOLE_SIZE
			}
		};

		const VkDescriptorImageInfo imageInfo[] = {
			{ sampler,
			  imageViewObject,
			  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		};
		const VkWriteDescriptorSet descWrite[] = {
			{
			  VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			  nullptr,
			  descriptorSets[ i ],								 // descriptorSets
			  0,												 // dstBinding
			  0,												 //dstArrayElement
			  sizeof( bufferInfo ) / sizeof( bufferInfo[ 0 ] ),	 // descriptorCount
			  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				 // type
			  nullptr,											 // image
			  bufferInfo,										 //buffer
			  nullptr											 // texelBufferView
			},
			{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			  nullptr,
			  descriptorSets[ i ],
			  1,
			  0,
			  sizeof( imageInfo ) / sizeof( imageInfo[ 0 ] ),
			  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			  imageInfo,
			  nullptr,
			  nullptr }
		};
		vkUpdateDescriptorSets( *device, 2, descWrite, 0, nullptr );
	}

	context->DescriptorSets = std::move( descriptorSets );

	VkPipelineVertexInputStateCreateInfo vertCreateInfo = {};
	vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertCreateInfo.pNext = nullptr;
	vertCreateInfo.pVertexAttributeDescriptions = inputAttribDesc;	//
	vertCreateInfo.pVertexBindingDescriptions = inputBindingDesc;	// stride
	vertCreateInfo.vertexAttributeDescriptionCount = sizeof( inputAttribDesc ) / sizeof( inputAttribDesc[ 0 ] );
	vertCreateInfo.vertexBindingDescriptionCount = sizeof( inputBindingDesc ) / sizeof( inputBindingDesc[ 0 ] );

	VkPipelineInputAssemblyStateCreateInfo asmCreateInfo = {};	// what to draw  glDrawArrays(GL_TRIANGLES)
	asmCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	asmCreateInfo.pNext = nullptr;
	asmCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	asmCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Default setting according to context->Swapchain
	VkViewport viewport;
	viewport.width = context->Swapchain->Size.width;
	viewport.height = context->Swapchain->Size.height;
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = context->Swapchain->Size;

	VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.pNext = nullptr;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pScissors = &scissor;
	viewportCreateInfo.scissorCount = 1;

	/////
	const VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {

		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_TRUE,			 // enable depth test
		VK_TRUE,			 // enable depth write
		VK_COMPARE_OP_LESS,	 // op
		VK_FALSE,			 // depth bound test
		VK_FALSE,			 // enable stencil test
		{},					 // front
		{},					 // back
		0.0f,				 // minDepthBounds
		1.0f				 // maxDepthBounds
	};

	VkPipelineRasterizationStateCreateInfo rasterCreateInfo = {};
	rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterCreateInfo.pNext = nullptr;
	rasterCreateInfo.depthClampEnable = VK_FALSE;
	rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterCreateInfo.lineWidth = 1.0f;
	rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	rasterCreateInfo.depthBiasEnable = VK_FALSE;
	rasterCreateInfo.depthBiasConstantFactor = 0.f;
	rasterCreateInfo.depthBiasClamp = 0.f;
	rasterCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multCreateInfo = {};
	multCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multCreateInfo.pNext = nullptr;
	multCreateInfo.sampleShadingEnable = VK_TRUE;
  multCreateInfo.minSampleShading = .2f;
	multCreateInfo.rasterizationSamples = context->NumSamples;
	multCreateInfo.minSampleShading = 1.f;
	multCreateInfo.pSampleMask = nullptr;
	multCreateInfo.alphaToOneEnable = VK_FALSE;
	multCreateInfo.alphaToCoverageEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState blendAttachmentState;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_R_BIT |
										  VK_COLOR_COMPONENT_B_BIT |
										  VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo bsCI = {};
	bsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	bsCI.pNext = nullptr;
	bsCI.pAttachments = &blendAttachmentState;
	bsCI.attachmentCount = 1;
	bsCI.logicOpEnable = VK_FALSE;
	bsCI.logicOp = VK_LOGIC_OP_COPY;
	bsCI.blendConstants[ 0 ] = 0.0f;
	bsCI.blendConstants[ 1 ] = 0.0f;
	bsCI.blendConstants[ 2 ] = 0.0f;
	bsCI.blendConstants[ 3 ] = 0.0f;

	VkDynamicState dyState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dyCI = {};
	dyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dyCI.pNext = nullptr;
	dyCI.pDynamicStates = dyState;
	dyCI.dynamicStateCount = 2;

	// Important: Uniforms
	VkPipelineLayoutCreateInfo plCI = {};
	plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plCI.pNext = nullptr;
	plCI.setLayoutCount = 1;
	VkDescriptorSetLayout desc[] = { setlayout };
	plCI.pSetLayouts = desc;
	plCI.pushConstantRangeCount = 0;
	plCI.pPushConstantRanges = 0;

	context->PipelineLayout = device->CreatePipelineLayout( plCI );

	const vector<VkAttachmentDescription> attachmentsDesc = {
		{
		  0,										 // flags
		  context->Swapchain->SurfaceFormat.format,	 // format
      context->NumSamples,
		  VK_ATTACHMENT_LOAD_OP_CLEAR,				 // loadOp
		  VK_ATTACHMENT_STORE_OP_STORE,				 // storeOp
		  VK_ATTACHMENT_LOAD_OP_DONT_CARE,			 // stencilLoadOp
		  VK_ATTACHMENT_STORE_OP_DONT_CARE,			 // stencilStoreOp
		  VK_IMAGE_LAYOUT_UNDEFINED,				 // initialLayout
		  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			 // finalLayout
		},
		{ 0,
		  device->PhysicalDevice->FindDepthFormat(),
		  context->NumSamples,
		  VK_ATTACHMENT_LOAD_OP_CLEAR,
		  VK_ATTACHMENT_STORE_OP_DONT_CARE,	 // We don't need the exact data
		  VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		  VK_ATTACHMENT_STORE_OP_DONT_CARE,
		  VK_IMAGE_LAYOUT_UNDEFINED,
		  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
    {
      0,
      context->Swapchain->SurfaceFormat.format,
      VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    }
	};

	const vector<VkAttachmentReference> colorAttacRef = {
		{
		  0,										// attachment index
		  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// layout
		},
	};
	const VkAttachmentReference depthAttachRef = {
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
  const VkAttachmentReference resolveAttachRef={
    2,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };
	const vector<VkSubpassDescription> triangleSubPass = {
		{ 0,
		  VK_PIPELINE_BIND_POINT_GRAPHICS,
		  0,
		  nullptr,
		  uint32_t( colorAttacRef.size() ),
		  colorAttacRef.data(),
      &resolveAttachRef, // resolve attachment
		  &depthAttachRef,
		  0, // preserve attachment 
		  nullptr }
	};

	const vector<VkSubpassDependency> dependency = {
		{ // I don't understand this dependency configuration
		  VK_SUBPASS_EXTERNAL,
		  0,
		  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		  0,
		  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		  0 }
	};
	// Important: Use of attachment
	const VkRenderPassCreateInfo rpCI = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		uint32_t( attachmentsDesc.size() ),
		attachmentsDesc.data(),
		uint32_t( triangleSubPass.size() ),
		triangleSubPass.data(),
		uint32_t( dependency.size() ),
		dependency.data()
	};

	auto renderPass = device->CreateRenderPass( rpCI );
	context->RenderPass = std::move( renderPass );

	VkGraphicsPipelineCreateInfo gpCI = {};
	gpCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	gpCI.pNext = nullptr;
	gpCI.stageCount = 2;
	gpCI.pStages = shaderStageCreateInfos;	// shaders
	gpCI.pVertexInputState = &vertCreateInfo;
	gpCI.pInputAssemblyState = &asmCreateInfo;
	gpCI.pViewportState = &viewportCreateInfo;
	gpCI.pRasterizationState = &rasterCreateInfo;
	gpCI.pMultisampleState = &multCreateInfo;
	gpCI.pDepthStencilState = &depthStencilStateCI;
	gpCI.pColorBlendState = &bsCI;
	gpCI.pDynamicState = nullptr;
	gpCI.layout = context->PipelineLayout;	// uniforms
	gpCI.renderPass = context->RenderPass;
	gpCI.subpass = 0;  // index
	gpCI.basePipelineHandle = VK_NULL_HANDLE;
	gpCI.basePipelineIndex = -1;
  

	context->RenderArea = context->Swapchain->Size;
	auto pipeline = device->CreatePipeline( gpCI );
	context->Pipeline = std::move( pipeline );

	context->CreateFramebuffers();

	context->VertexBuffer = std::move( vertexBuffer );
	context->CreateCommandList();

	ubo.view = glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ),
							glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

	ubo.proj = glm::perspective( glm::radians( 45.0f ),
								 1.0f * context->Swapchain->Size.width / context->Swapchain->Size.height,
								 0.1f, 10.0f );

	ubo.proj[ 1 ][ 1 ] *= -1;

	while ( glfwWindowShouldClose( surface->Window.get() ) == false ) {
		// Update Uniform matrix
		ubo.model = UpdateModelMatrix();

		for ( int i = 0; i < swapchainImageCount; i++ ) {
			uboBuffers[ i ].SetData( &ubo, sizeof( UniformBufferObject ) );
		}

		context->SubmitCommandList();
		glfwPollEvents();
	}

	vkDeviceWaitIdle( *device );
	return 0;
}
