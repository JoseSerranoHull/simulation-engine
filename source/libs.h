#pragma once

/**
 * @file libs.h
 * @brief Gateway header for external library dependencies.
 */

 /* parasoft-begin-suppress ALL */

 // --- Standard C++ Libraries ---
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <fstream>
#include <limits>
#include <cstdint>

// --- GLM Configuration for Vulkan ---
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- GLFW & Vulkan ---
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// --- ImGui Core & Backends ---
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

/* parasoft-end-suppress ALL */