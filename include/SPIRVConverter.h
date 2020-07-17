
#pragma once

#include <vector>
#include <string>

class SPIRVConverter
{
  public:
    enum class ShaderType{
      Vertex,
      TessllationControl,
      TessllationEvaluate,
      Geometry,
      Fragment,
      Compute
    };
    std::vector<unsigned int> GLSL2SPRIV(const std::string & fileName);
    std::vector<unsigned int> GLSL2SPRIVFromFile(const std::string & fileName,ShaderType shaderType);
    std::vector<unsigned int> GLSL2SPRIVFromSource(const std::string & source,ShaderType shaderType);
};
