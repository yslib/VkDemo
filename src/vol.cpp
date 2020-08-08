#include <VKContext.hpp>
#include <glm/fwd.hpp>
#include <vector>
#include <fstream>
#include <vulkan/vulkan_core.h>
#include <SPIRVConverter.h>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

struct UBO{
  mat4x4 model;
  mat4x4 view;
  mat4x4 proj;
};

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
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	context->VertexBuffer = device->CreateBuffer( vbci, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	context->CopyBuffer( vertexStageBuffer, context->VertexBuffer, vsbci.size );

	// create index buffer
	const VkBufferCreateInfo isbci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		indices.size() * sizeof( unsigned int ),
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
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	context->IndexBuffer = device->CreateBuffer( ibci, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	context->CopyBuffer( indexStagingBuffer, context->IndexBuffer, isbci.size );
	context->VertexCount = indices.size();

	// load volume data
	uint32_t width = 160, height = 240, depth = 40;
	const auto total = width * height * depth;

	auto file = ifstream( "resources/mixfrac160x240x40.raw", ios::binary );
	if ( file.is_open() == false ) {
		cout << "failed to open volume data file\n";
		exit( -1 );
	}
	vector<char> d( total );
	file.read( d.data(), total );

	VkBufferCreateInfo vdci = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>( total ),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	auto volumeStagingBuffer = device->CreateBuffer( vdci, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
	volumeStagingBuffer.SetData( d.data(), total );

	// Create image object
	const VkImageCreateInfo ici = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_3D,
		VK_FORMAT_R8_UNORM,
		{ width, height, depth },
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	auto volumeImage = device->CreateImage( ici );
	auto volumeImageView = volumeImage.CreateImageView( VK_FORMAT_R8_UNORM, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT );

	cout << "Create Volume Texture\n";


  // create uniform buffer for cubevert 
  //
	VkDeviceSize uboSize = sizeof( UBO );
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
	for ( int i = 0; i < context->Swapchain->SwapchainImageViews.size(); i++ ) 
  {
		uboBuffers.push_back( device->CreateBuffer( uboBufCI, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) );
	}

	// Creates pipeline
	// [0] Shaders

	VkPipelineShaderStageCreateInfo pass1Shaders[ 2 ] = {};

	// position shader
	{
		const auto vShader = SPIRVConverter().GLSL2SPRIVFromFile( "shader/cubevert.glsl", SPIRVConverter::ShaderType::Vertex );
		const auto fShader = SPIRVConverter().GLSL2SPRIVFromFile( "shader/cubefrag.glsl", SPIRVConverter::ShaderType::Fragment );

		VkShaderModuleCreateInfo vsCI = {};
		vsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vsCI.pNext = nullptr;
		vsCI.codeSize = vShader.size() * sizeof( unsigned int );
		vsCI.pCode = (uint32_t *)vShader.data();


		VkShaderModuleCreateInfo fsCI = {};
		fsCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		fsCI.pNext = nullptr,
		fsCI.codeSize = fShader.size() * sizeof( unsigned int ),
		fsCI.pCode = (uint32_t *)( fShader.data() );

		auto vs = device->CreateShader( vsCI );
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
		fssCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fssCI.module = fs;
		fssCI.pName = "main";

    pass1Shaders[0] = vssCI;
    pass1Shaders[1] = fssCI;
	}


  // attrib binding --> attribute desc
  //    |----------------------------------------|
  //    |      |--------------|                  |
  //    |      | attribBinding|                  |
  //    |      |--------------|                  |
  //    |     attribute desc                     |
  //    |----------------------------------------|
  //

  // Vertex Attribute binding
  //
  const array<VkVertexInputBindingDescription,1> ibDesc={
    {0, // binding point
      3 * sizeof(float),  //stride
      VK_VERTEX_INPUT_RATE_VERTEX
    }
  };

  const array<VkVertexInputAttributeDescription,1> iaDesc={
    {
      0,
      0,  // the index of the binding point would be bound in the ibDesc
      VK_FORMAT_R32G32B32_SFLOAT,
      0
    }
  };


  /*
   * binding --> setlayout --> set
   * */

  // Descirptor layout binding : a single uniform variable in shader
  const array<VkDescriptorSetLayoutBinding,1> bindings{
    { 
      0, // binding point
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // type
      1,                                 // count
      VK_SHADER_STAGE_VERTEX_BIT,        // stage
      nullptr                            // immutable sampler
    }
  };

  // descriptor set layout 

  VkDescriptorSetLayoutCreateInfo dslCI={
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    nullptr,
    0,
    bindings.size(),
    bindings.data()
  };

  const uint32_t swapchainImageCount = context->Swapchain->SwapchainImages.size();

  auto layout = device->CreateDescriptorSetLayout( dslCI );

  // Given a setlayout for each swapchain image
  vector<VkDescriptorSetLayout> layouts( swapchainImageCount, layout );

  // descriptor pool 
  

  const array<VkDescriptorPoolSize,1> poolSize
  {
    {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      swapchainImageCount               // the number of certern type descriptor(uniform)
    }
  };

  VkDescriptorPoolCreateInfo dpCI=
  {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    nullptr,
    0,
    swapchainImageCount,               // the max number of allocatable set
    poolSize.size(),
    poolSize.data()
  };
  context->DescriptorPool = device->CreateDescriptorPool(dpCI);
  vector<VkDescriptorSet> sets(swapchainImageCount);
  VkDescriptorSetAllocateInfo dsAI = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    nullptr,
    context->DescriptorPool,
    swapchainImageCount,
    layouts.data()
  };

  VK_CHECK(vkAllocateDescriptorSets(*device,&dsAI,sets.data()));

  // update data for each set
  for(int i = 0;i<swapchainImageCount;i++){
    const array<VkDescriptorBufferInfo,1> bufferInfo{
      uboBuffers[i], // VkBuffer
      0,          // offset
      sizeof(UBO) // equal to VK_WHOLE_SIZE
    };
    
    const array<VkDescriptorImageInfo,0> imageInfo{
      //sampler
      //imageview
      //imagelayout
    };

    const array<VkWriteDescriptorSet,1> write
    {
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        sets[i],
        0,
        0,
        bufferInfo.size(),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        nullptr,
        bufferInfo.data(),
        nullptr
      }
    };
    vkUpdateDescriptorSets(*device,write.size(),write.data(),0,nullptr);
  }
  context->DescriptorSets = std::move(sets);

  // create pipeline VkPipeline prefix and State suffix

  //[1] Vertex input
  VkPipelineVertexInputStateCreateInfo isCI={};
  isCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  isCI.pNext = nullptr;
  isCI.pVertexAttributeDescriptions = iaDesc.data();
  isCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(iaDesc.size());
  isCI.pVertexBindingDescriptions = ibDesc.data();
  isCI.vertexBindingDescriptionCount = static_cast<uint32_t>(ibDesc.size());

  //[2] primitive
  VkPipelineInputAssemblyStateCreateInfo iaCI={};
	iaCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaCI.pNext = nullptr;
	iaCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	iaCI.primitiveRestartEnable = VK_FALSE;

  //[3] viewport
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


  //[4] depth stencil   (glEnable)
	const VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = 
  {
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

  //[5] multisample
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


  //[6] blend state
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

  // [7] dynamic state
  const array<VkDynamicState,2> dyState{
    VK_DYNAMIC_STATE_VIEWPORT};

  VkPipelineDynamicStateCreateInfo dsCI = {};
  dsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dsCI.pNext = nullptr;
  dsCI.pDynamicStates = dyState.data();
  dsCI.dynamicStateCount = dyState.size();


	VkPipelineLayoutCreateInfo plCI = {};
	plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plCI.pNext = nullptr;
	plCI.setLayoutCount = 1;
	plCI.pSetLayouts = &layout;
	plCI.pushConstantRangeCount = 0;
	plCI.pPushConstantRanges = 0;

  auto positionPipelineLayout = device->CreatePipelineLayout(plCI);

  // render pass

  






  return 0;
}
