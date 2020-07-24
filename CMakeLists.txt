cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(VkDemo)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(3rdparty/glslang)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(mesh)
target_sources(mesh PRIVATE "src/mesh.cpp" "src/SPIRVConverter.cpp")
target_link_libraries(mesh ${Vulkan_LIBRARIES} glfw glm glslang SPIRV)
target_include_directories(mesh  PRIVATE ${Vulkan_INCLUDE_DIRS} "3rdparty" "include")



add_executable(vol)
target_sources(vol PRIVATE "src/SPIRVConverter.cpp" "src/vol.cpp")
target_link_libraries(vol ${Vulkan_LIBRARIES} glfw glm glslang SPIRV)
target_include_directories(vol  PRIVATE ${Vulkan_INCLUDE_DIRS} "3rdparty" "include")

# copy the shader to the install directory
#install(TARGETS ${PROJ_NAME} LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")

install(DIRECTORY "shader" DESTINATION "bin")


set(TEST_SPIRV On)
if(TEST_SPIRV)
  add_subdirectory(test)
endif()
