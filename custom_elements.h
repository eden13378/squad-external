#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <map>
#define  IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

using namespace std;

class custom_elements {

public:

	bool Checkbox(const char* label, bool* v);

	bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);

	bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	bool Keybind(const char* label, int* key, bool widget_label);

};

inline custom_elements custom;