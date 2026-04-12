cmake_minimum_required(VERSION 3.24)

project(VulkanEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.4 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

find_program(SLANGC_EXECUTABLE
	NAMES slangc
	HINTS
		"$ENV{VULKAN_SDK}/bin"
		"$ENV{VULKAN_SDK}/macOS/bin"
)

if (NOT SLANGC_EXECUTABLE)
	message(FATAL_ERROR "slangc was not found. Set VULKAN_SDK or add slangc to PATH.")
endif()

function(add_slang_shader_target TARGET)
	cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})

	if (NOT SHADER_SOURCES)
		message(FATAL_ERROR "add_slang_shader_target(${TARGET}) requires SOURCES.")
	endif()

	set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
	set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${TARGET}.spv)
	set(ENTRY_POINTS
		-entry vertMain
		-entry fragMain
	)

	add_custom_command(
		OUTPUT ${OUTPUT_FILE}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
		COMMAND ${SLANGC_EXECUTABLE}
			${SHADER_SOURCES}
			-target spirv
			-profile spirv_1_4
			-emit-spirv-directly
			-fvk-use-entrypoint-name
			${ENTRY_POINTS}
			-o ${OUTPUT_FILE}
		DEPENDS ${SHADER_SOURCES}
		COMMENT "Compiling Slang shaders for ${TARGET}"
		VERBATIM
	)

	add_custom_target(${TARGET}_shaders DEPENDS ${OUTPUT_FILE})
endfunction()

add_executable(VulkanEngine
	AzimuthEngine/src/main.cpp
	AzimuthEngine/src/Application.cpp
	AzimuthEngine/src/log/log.cpp
	AzimuthEngine/src/window/GLFWWindow.cpp
	AzimuthEngine/src/window/GLFWWindowManager.cpp
	AzimuthEngine/src/backend/Vulkan/VkCore.cpp
	AzimuthEngine/src/backend/Vulkan/VkLogicalDevice.cpp
	AzimuthEngine/src/backend/Vulkan/VkPhysicalDevice.cpp
	AzimuthEngine/src/backend/Vulkan/VkSwapChain.cpp
)

add_slang_shader_target(VulkanEngine
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AzimuthEngine/shaders/shader.slang
)

add_dependencies(VulkanEngine VulkanEngine_shaders)

target_include_directories(VulkanEngine
	PRIVATE
	AzimuthEngine/inc/AzimuthEngine
	AzimuthEngine/inc/AzimuthEngine/log
	AzimuthEngine/inc/AzimuthEngine/window
	AzimuthEngine/inc/AzimuthEngine/event
	AzimuthEngine/inc/AzimuthEngine/core
	AzimuthEngine/inc/AzimuthEngine/backend/Vulkan
)

target_link_libraries(VulkanEngine
	PRIVATE
	Vulkan::Vulkan
	glfw
	glm::glm
	spdlog::spdlog
)
