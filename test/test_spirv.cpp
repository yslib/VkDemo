#include <gtest/gtest.h>
#include <vector>
#include <fstream>
#include "../include/SPIRVConverter.h"
using namespace std;
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

TEST(test_spriv, basic)
{
  auto cvt = SPIRVConverter();
  auto vert1 = OpenSprivFromFile("shader/vert.spv");
  auto vert2 = cvt.GLSL2SPRIVFromFile("shader/test.vert",SPIRVConverter::ShaderType::Vertex);
  std::cout<<vert1.size()<<" "<<vert2.size()<<std::endl;
}
