#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include "ImguiUtil.h"
#include "Physics.h"
#include "Camera.h"

class UserInterface
{
public:
	UserInterface() {};
	~UserInterface() {};

	void InitUserInterface(GLFWwindow * window);
	void ShowMainUi(Physics* physics, Camera* camera, float* xTranslate, float* yTranslate, float* zTranslate);
	std::vector<std::string> GetAllFilesInFolder(std::string folderPath);
	std::vector<std::string> GetAllFoldersInFolder(std::string folderPath);

	bool isPaused = true;
	bool showMainWindow = true;
	bool showLoadPopup = false;
	bool showSavePopup = false;
	std::vector<std::string> saveFiles;
	std::vector<bool> selected;
	std::vector<std::string> textureFolders;
	bool showTopLeftOverlay = true;


private:
	bool DataTypeApplyOpFromTextScientific(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* scalar_format);
	bool InputScalarScientific(const char* label, ImGuiDataType data_type, void* data_ptr, const char* scalar_format, ImGuiInputTextFlags extra_flags);
	bool InputScientific(const char* label, float* v, const char *display_format = "%.3g", ImGuiInputTextFlags extra_flags = 0);

	template <UnitType type> bool UnitCombo(std::string id, ValueWithUnits<type>* value);
	template <UnitType type> bool UnitCombo3(std::string id, ValueWithUnits3<type>* value);
	template <typename T> T clip(const T& n, const T& lower, const T& upper);

	void LoadPopup(Physics* physics);
	void TopLeftOverlay(Physics* physics);
	void SavePopup(Physics* physics);
	void MenuBar(Physics* physics);
};
#endif