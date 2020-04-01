
#pragma once

#include <vector>
#include <string>

class SPIRVConverter{
  public:
    std::vector<unsigned int> GLSL2SPRIV(const std::string & fileName);
};
