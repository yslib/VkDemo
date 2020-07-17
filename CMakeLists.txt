cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJ_NAME "VkDemo")
project(${PROJ_NAME})

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(3rdparty/glslang)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

aux_source_directory("src" SRC)

add_executable(${PROJ_NAME})
target_sources(${PROJ_NAME} PRIVATE ${SRC})
target_link_libraries(${PROJ_NAME} ${Vulkan_LIBRARIES} glfw glm glslang SPIRV)
target_include_directories(${PROJ_NAME}  PRIVATE ${Vulkan_INCLUDE_DIRS} "3rdparty" "include")

# copy the shader to the install directory
install(TARGETS ${PROJ_NAME} LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(DIRECTORY "shader" DESTINATION "bin")

set(TEST_SPIRV On)
if(TEST_SPIRV)
  add_subdirectory(test)
endif()
