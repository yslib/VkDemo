cmake_minimum_required(VERSION 3.8)
project(vlkdemo)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)


add_subdirectory(3rdparty/glslang)


find_package(Vulkan REQUIRED)


find_package(glfw3 CONFIG REQUIRED)
if(glfw3_FOUND)
message(STATUS "GLFW3 Found")
endif()

find_package(glm CONFIG REQUIRED)

add_executable(vlkdemo )
target_sources(vlkdemo PRIVATE "main.cpp")
target_link_libraries(vlkdemo ${Vulkan_LIBRARIES} glfw glm glslang SPIRV)
target_include_directories(vlkdemo PRIVATE ${Vulkan_INCLUDE_DIRS})
#target_include_directories(vlkdemo PRIVATE ${GLSLANG_INCLUDE_DIRS})


# copy the shader to the install directory
install(TARGETS vlkdemo LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(DIRECTORY "shader" DESTINATION "bin")
