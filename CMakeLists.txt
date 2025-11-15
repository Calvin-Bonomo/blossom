cmake_minimum_required(VERSION 3.3...4.0)

project(Blossom VERSION 0.1)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.4 REQUIRED)

if ( ${Vulkan_VERSION} VERSION_LESS "1.3.256")
    message(FATAL_ERROR "Minimum required Vulkan version for C++ module support is 1.3.256. "
    "Found ${Vulkan_VERSION}."
    )
endif()

add_library( VulkanHppModule )
target_sources( VulkanHppModule PRIVATE
    FILE_SET CXX_MODULES
    BASE_DIRS ${Vulkan_INCLUDE_DIR}
    FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)
target_compile_features(VulkanHppModule PUBLIC cxx_std_20)
target_link_libraries(VulkanHppModule PUBLIC Vulkan::Vulkan)

add_executable(${PROJECT_NAME} src/app.cpp src/app.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw VulkanHppModule)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

