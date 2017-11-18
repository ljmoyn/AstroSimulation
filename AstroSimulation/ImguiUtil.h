#ifndef IMGUI_UTIL_H
#define IMGUI_UTIL_H

// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "Physics.h"
#include "Camera.h"
#include <Windows.h>

#include <vector>

struct GLFWwindow;

IMGUI_API bool        ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks);
IMGUI_API void        ImGui_ImplGlfwGL3_Shutdown();
IMGUI_API void        ImGui_ImplGlfwGL3_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGlfwGL3_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGlfwGL3_CreateDeviceObjects();

// GLFW callbacks (installed by default if you enable 'install_callbacks' during initialization)
// Provided here if you want to chain callbacks.
// You can also handle inputs yourself and use those as a reference.
IMGUI_API void        ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_API void        ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_API void        ImGui_ImplGlfwGL3_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_API void        ImGui_ImplGlfwGL3_CharCallback(GLFWwindow* window, unsigned int c);

struct ImguiStatus {
	bool isPaused = true;
	bool showMainWindow = true;
	bool showLoadPopup = false;
	bool showSavePopup = false;
	std::vector<std::string> saveFiles;
	std::vector<bool> selected;
	std::vector<std::string> textureFolders;

	bool showTopLeftOverlay = true;

	//http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	//https://stackoverflow.com/questions/2239872/how-to-get-list-of-folders-in-this-folder
	//https://msdn.microsoft.com/en-us/library/aa365200(VS.85).aspx
	//windows specific
	std::vector<std::string> GetAllFilesInFolder(std::string folderPath)
	{
		std::vector<std::string> names;
		std::string search_path = folderPath + "/*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					names.push_back(fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}
		return names;
	}

	std::vector<std::string> GetAllFoldersInFolder(std::string folderPath)
	{
		std::vector<std::string> names;
		std::string search_path = folderPath;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (std::string)(fd.cFileName) != "." && (std::string)(fd.cFileName) != "..") {
					names.push_back(fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}
		return names;
	}
};

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

// Menus / Windows
IMGUI_API void ShowMainUi(Physics* physics, Camera* camera, ImguiStatus* imguiStatus, float* xTranslate, float* yTranslate, float* zTranslate);
IMGUI_API void MenuBar(Physics* physics, ImguiStatus* imguiStatus);
IMGUI_API void TopLeftOverlay(Physics* physics);
IMGUI_API void LoadPopup(Physics* physics, ImguiStatus* imguiStatus);
IMGUI_API void SavePopup(Physics* physics, ImguiStatus* imguiStatus);

// Widgets
IMGUI_API bool InputScientific(const char* label, float* v, const char *display_format = "%.3g", ImGuiInputTextFlags extra_flags = 0);
IMGUI_API template <UnitType type> bool UnitCombo(std::string id, ValueWithUnits<type>* value);
IMGUI_API template <UnitType type> bool UnitCombo3(std::string id, ValueWithUnits3<type>* value);

//https://eliasdaler.github.io/using-imgui-with-sfml-pt2#combobox-listbox
static auto vector_getter = [](void* vec, int idx, const char** out_text)
{
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
};


#endif