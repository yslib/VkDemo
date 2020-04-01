
#include <iostream>
#include <fstream>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>

#include "SPIRV/SpvTools.h"
#include "SPIRVConverter.h"
#include "StandAlone/ResourceLimits.h"

const TBuiltInResource MyDefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};


std::vector<unsigned int> SPIRVConverter::GLSL2SPRIV(const std::string & fileName){

  using namespace std;
	using namespace std::filesystem;
	using namespace glslang;

	ifstream glsl(fileName);
	if (glsl.is_open() == false)
	{
		cout << "Failed to open glsl file\n";
		throw runtime_error("Failed to open glsl file");
	}
	
	string glslCode{ istreambuf_iterator<char>{glsl}, istreambuf_iterator<char>{} };


	EShLanguage shaderType;

	const string extension = path(fileName).extension().string();

	if(extension ==".vert")
	{
		shaderType = EShLangVertex;
	}else if(extension ==".tesc")
	{
		shaderType = EShLangTessControl;
	}else if(extension == ".tese")
	{
		shaderType = EShLangTessEvaluation;
	}else if(extension == ".geom")
	{
		shaderType = EShLangGeometry;
	}else if(extension == ".frag")
	{
		shaderType = EShLangFragment;
	}else if(extension == ".comp")
	{
		shaderType = EShLangCompute;
	}else
	{
		cout << "Unknown shader type: "<<extension<<endl;
		return {};
	}

  InitializeProcess();
  

	TShader shader(shaderType);

	const char* code = glslCode.c_str();
	shader.setStrings(&code, 1);

	int ClientInputSemanticsVersion = 100;
	EShTargetClientVersion vkVersion = EShTargetVulkan_1_1;
	EShTargetLanguageVersion spirvVersion = EShTargetSpv_1_0;

	shader.setEnvInput(EShSourceGlsl, shaderType, EShClientVulkan, ClientInputSemanticsVersion);
	shader.setEnvClient(EShClientVulkan, vkVersion);
	shader.setEnvTarget(EshTargetSpv, spirvVersion);

  TBuiltInResource Resources;
  Resources  = MyDefaultTBuiltInResource ;
	EShMessages msg = (EShMessages)(EShMsgSpvRules|EShMsgVulkanRules);

	const int defaultVersion = 100;

	DirStackFileIncluder includer;


	const string p = path(fileName).parent_path().string();
	includer.pushExternalLocalDirectory(p);

	string preprocessedGLSL;

	if(shader.preprocess(&Resources,defaultVersion,ENoProfile,false,false,msg,&preprocessedGLSL,includer) == false)
	{
		cout << "GLSL preprocessing failed for: " << fileName << endl;
		cout << shader.getInfoLog() << endl;
		cout << shader.getInfoDebugLog() << endl;
	}

	const char* preproccesdGLSLStr = preprocessedGLSL.c_str();
	shader.setStrings(&preproccesdGLSLStr,1);

  if(shader.parse(&Resources,defaultVersion,false,msg) == false){
    std::cout<<"GLSL Parse error: "<<fileName<<std::endl;
    std::cout<<shader.getInfoLog()<<std::endl;
    std::cout<<shader.getInfoDebugLog()<<std::endl;
  }

  TProgram program;
  program.addShader(&shader);
  if(program.link(msg) == false){
    std::cout<<"Linking Program failed: "<<fileName<<std::endl;
    std::cout<<shader.getInfoLog()<<std::endl;
    std::cout<<shader.getInfoDebugLog()<<std::endl;

  }

  std::vector<unsigned int> spirv;
  spv::SpvBuildLogger logger;
  SpvOptions opt;
  GlslangToSpv(*program.getIntermediate(shaderType),spirv,&logger,&opt);
  return spirv;
}
