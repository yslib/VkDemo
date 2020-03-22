//#include <StandAlone/DirStackFileIncluder.h>
#include "VKContext.hpp"
#include <bits/stdint-uintn.h>
#include <limits>
#include <memory>
#include <vulkan/vulkan_core.h>
#include "SPIRVConverter.h"

std::vector<char> OpenSprivFromFile( const std::string &fileName )
{
	std::ifstream fileIn( fileName, std::ios::binary | std::ios::ate );
	if ( fileIn.is_open() == false ) {
		std::cout << "Failed to open SPIRV: " << fileName << std::endl;
		exit( -1 );
	}
	std::vector<char> code( fileIn.tellg() );
	fileIn.seekg( 0, std::ios::beg );
	fileIn.read( code.data(), code.size() );
	return code;
}

using namespace std;
int main()
{
	auto instance = std::make_shared<VkInstanceObject>();
	auto physicalDevice = std::make_shared<VkPhysicalDeviceObject>( instance );
	auto device = std::make_shared<VkDeviceObject>( physicalDevice );
	auto surface = std::make_shared<VkSurfaceObject>( instance, device );
	auto swapchain = std::make_shared<VkSwapchainObject>( device, surface );
	//
	//
	// SPIRVConverter test
	//SPIRVConverter cvt;
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

	VkPipelineVertexInputStateCreateInfo vertCreateInfo = {};
	vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertCreateInfo.pNext = nullptr;
	vertCreateInfo.pVertexAttributeDescriptions = nullptr;
	vertCreateInfo.pVertexBindingDescriptions = nullptr;
	vertCreateInfo.vertexAttributeDescriptionCount = 0;
	vertCreateInfo.vertexBindingDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo asmCreateInfo = {};	// what to draw  glDrawArrays(GL_TRIANGLES)
	asmCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	asmCreateInfo.pNext = nullptr;
	asmCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	asmCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Default setting according to swapchain
	VkViewport viewport;
	viewport.width = swapchain->Size.width;
	viewport.height = swapchain->Size.height;
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain->Size;

	VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.pNext = nullptr;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pScissors = &scissor;
	viewportCreateInfo.scissorCount = 1;

	/////

	VkPipelineRasterizationStateCreateInfo rasterCreateInfo = {};
	rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterCreateInfo.pNext = nullptr;
	rasterCreateInfo.depthClampEnable = VK_FALSE;
	rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterCreateInfo.lineWidth = 1.0f;
	rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterCreateInfo.depthBiasEnable = VK_FALSE;
	rasterCreateInfo.depthBiasConstantFactor = 0.f;
	rasterCreateInfo.depthBiasClamp = 0.f;
	rasterCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multCreateInfo = {};
	multCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multCreateInfo.pNext = nullptr;
	multCreateInfo.sampleShadingEnable = VK_FALSE;
	multCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
	plCI.pSetLayouts = nullptr;
	plCI.pushConstantRangeCount = 0;
	plCI.setLayoutCount = 0;
	plCI.pPushConstantRanges = 0;

	auto pipelineLayout = device->CreatePipelineLayout( plCI );

	const VkAttachmentDescription colorAttachDesc[] = {
		{ 0,
		  swapchain->SurfaceFormat.format,
		  VK_SAMPLE_COUNT_1_BIT,
		  VK_ATTACHMENT_LOAD_OP_CLEAR,
		  VK_ATTACHMENT_STORE_OP_STORE,
		  VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		  VK_ATTACHMENT_STORE_OP_DONT_CARE,
		  VK_IMAGE_LAYOUT_UNDEFINED,
		  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
	};

	const VkAttachmentReference colorAttacRef = {
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription triangleSubPass = {
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		1,
		&colorAttacRef,
		nullptr,
		nullptr,
		0,
		nullptr
	};

	VkSubpassDependency dependency = { // I don't understand this dependency configuration
									   VK_SUBPASS_EXTERNAL,
									   0,
									   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
									   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
									   0,
									   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
									   0
	};
	// Important: Use of attachment
	VkRenderPassCreateInfo rpCI = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		1,
		colorAttachDesc,
		1,
		&triangleSubPass,
		1,
		&dependency,
	};
	auto renderpass = device->CreateRenderPass( rpCI );

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
	gpCI.pDepthStencilState = nullptr;
	gpCI.pColorBlendState = &bsCI;
	gpCI.pDynamicState = nullptr;
	gpCI.layout = pipelineLayout;  // uniforms
	gpCI.renderPass = renderpass;
	gpCI.subpass = 0;  // index
	gpCI.basePipelineHandle = VK_NULL_HANDLE;
	gpCI.basePipelineIndex = -1;

	auto pipeline = device->CreatePipeline( gpCI );


  // Create framebuffers
  // Framebuffer is coupled with renderpass

	std::vector<VkFramebufferObject> framebuffers( swapchain->SwapchainImageViews.size() );
	for ( int i = 0; i < swapchain->SwapchainImageViews.size(); i++ ) {
		VkImageView attachments[] = {
			swapchain->SwapchainImageViews[ i ]
		};
		VkFramebufferCreateInfo CI = {};
		CI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CI.pNext = nullptr;
		CI.renderPass = renderpass; // coupled
		CI.attachmentCount = 1;
		CI.pAttachments = attachments;
		CI.width = swapchain->Size.width;
		CI.height = swapchain->Size.height;
		CI.layers = 1;
		framebuffers[ i ] = device->CreateFramebuffer( CI );
	}

	VkCommandPoolCreateInfo cpCI = {};
	cpCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpCI.pNext = nullptr;
	cpCI.flags = 0;	 // Important
	auto commandPool = device->CreateCommandPool( cpCI );

	vector<VkCommandBuffer> cmdBuffers( framebuffers.size() );
	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.pNext = nullptr;
	cbAllocInfo.commandPool = commandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbAllocInfo.commandBufferCount = cmdBuffers.size();

	VK_CHECK( vkAllocateCommandBuffers( *device, &cbAllocInfo, cmdBuffers.data() ) );

	for ( size_t i = 0; i < cmdBuffers.size(); i++ ) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		VK_CHECK( vkBeginCommandBuffer( cmdBuffers[ i ], &beginInfo ) );

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.framebuffer = framebuffers[ i ]; // renderpass begininfo is coupled with framebuffer
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain->Size;
		renderPassInfo.renderPass = renderpass;

		VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass( cmdBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

		vkCmdBindPipeline( cmdBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
		vkCmdDraw( cmdBuffers[ i ], 3, 1, 0, 0 );

		vkCmdEndRenderPass( cmdBuffers[ i ] );

		VK_CHECK( vkEndCommandBuffer( cmdBuffers[ i ] ) );
	}
	// auto imageAvaliableSemaphore = device->CreateSemaphore();
	// auto renderFinishedSemaphore = device->CreateSemaphore();

	constexpr int flightImagesCount = 3;
	vector<VkSemaphoreObject> imageAvaliableSemaphores;
	vector<VkSemaphoreObject> renderFinishedSemaphores;
	vector<VkFenceObject> fences;

	for ( int i = 0; i < flightImagesCount; i++ ) {
		imageAvaliableSemaphores.push_back( device->CreateSemaphore() );
		renderFinishedSemaphores.push_back( device->CreateSemaphore() );
		fences.push_back( device->CreateFence() );
	}

	uint32_t currentFrameIndex = 0;

	while ( glfwWindowShouldClose( surface->Window.get() ) == false ) {
		VkFence waitFences = fences[ currentFrameIndex ];
		assert( waitFences != VK_NULL_HANDLE );
		VK_CHECK( vkWaitForFences( *device, 1, &waitFences, VK_TRUE, std::numeric_limits<uint64_t>::max() ) );
		VK_CHECK( vkResetFences( *device, 1, &waitFences ) );
		uint32_t imageIndex;
		vkAcquireNextImageKHR( *device, *swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliableSemaphores[ currentFrameIndex ], VK_NULL_HANDLE, &imageIndex );

		// There are serveral things that you need to present before submitting cmds
		// [1] semaphores which need to be done
		// [2] Pipeline Stage need to be waited
		// [3] command buffer need to be submitted
		// [4] semaphores need to signal after draw call finished

		VkSemaphore waitSemaphores[] = { imageAvaliableSemaphores[ currentFrameIndex ] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[ currentFrameIndex ] };
		const VkSubmitInfo submitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1,
			waitSemaphores,
			waitStages,
			1,
			&cmdBuffers[ imageIndex ],
			1,
			signalSemaphores
		};

		VK_CHECK( vkQueueSubmit( device->GraphicsQueue, 1, &submitInfo, fences[ currentFrameIndex ] ) );

		const VkSemaphore presentWatiSemaphore[] = { renderFinishedSemaphores[ currentFrameIndex ] };
		const VkSwapchainKHR swapchains[] = { *swapchain };
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

		VK_CHECK( vkQueuePresentKHR( device->GraphicsQueue, &presentInfo ) );
		currentFrameIndex = ( currentFrameIndex + 1 ) % flightImagesCount;
	}

	vkDeviceWaitIdle( *device );
	std::cout << "Create Frambuffer finished\n";
	return 0;
}
