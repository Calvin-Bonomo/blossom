cmake_minimum_required(VERSION 3.3...4.0)

project(Blossom VERSION 0.1)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.4 REQUIRED)

add_executable(${PROJECT_NAME} src/app.cpp src/app.hpp)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

