#include "UserInterface.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

void UserInterface::InitUserInterface(GLFWwindow * window)
{
	textureFolders = GetAllFoldersInFolder("../cubemaps/*");

	// Initialize ImGui
	ImGui_ImplGlfwGL3_Init(window, false);
	saveFiles = GetAllFilesInFolder("../saves");
	selected.resize(saveFiles.size());
	std::fill(selected.begin(), selected.end(), false);
	if (!selected.empty())
		selected[0] = true;
}

bool UserInterface::DataTypeApplyOpFromTextScientific(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* scalar_format)
{
	scalar_format = "%f";
	float* v = (float*)data_ptr;
	const float old_v = *v;
	*v = (float)atof(buf);
	return *v != old_v;
}

bool UserInterface::InputScalarScientific(const char* label, ImGuiDataType data_type, void* data_ptr, const char* scalar_format, ImGuiInputTextFlags extra_flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImGui::BeginGroup();
	ImGui::PushID(label);

	char buf[64];
	ImFormatString(buf, IM_ARRAYSIZE(buf), scalar_format, *(float*)data_ptr);

	bool value_changed = false;
	extra_flags |= ImGuiInputTextFlags_AutoSelectAll;
	if (ImGui::InputText("", buf, IM_ARRAYSIZE(buf), extra_flags))
		value_changed = DataTypeApplyOpFromTextScientific(buf, GImGui->InputTextState.InitialText.begin(), data_type, data_ptr, scalar_format);

	ImGui::PopID();

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		ImGui::RenderText(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), label);
		ImGui::ItemSize(label_size, style.FramePadding.y);
	}
	ImGui::EndGroup();

	return value_changed;
}

//https://github.com/ocornut/imgui/issues/648
bool UserInterface::InputScientific(const char* label, float* v, const char *display_format, ImGuiInputTextFlags extra_flags)
{
	return InputScalarScientific(label, ImGuiDataType_Float, (void*)v, display_format, extra_flags);
}

template <UnitType type> bool UserInterface::UnitCombo(std::string id, ValueWithUnits<type>* value) {
	bool changed = false;
	int units = value->unitIndex;
	switch (type)
	{
	case UnitType::Distance:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.positionUnits, IM_ARRAYSIZE(value->unitData.positionUnits));
		break;
	case UnitType::Velocity:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.velocityUnits, IM_ARRAYSIZE(value->unitData.velocityUnits));
		break;
	case UnitType::Mass:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.massUnits, IM_ARRAYSIZE(value->unitData.massUnits));
		break;
	case UnitType::Time:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.timeUnits, IM_ARRAYSIZE(value->unitData.timeUnits));
		break;
	}
	if (changed)
		value->ConvertToUnits(units);

	return changed;
}

template <UnitType type> bool UserInterface::UnitCombo3(std::string id, ValueWithUnits3<type>* value) {
	bool changed = false;
	int units = value->unitIndex;
	switch (type)
	{
	case UnitType::Distance:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.positionUnits, IM_ARRAYSIZE(value->unitData.positionUnits));
		break;
	case UnitType::Velocity:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.velocityUnits, IM_ARRAYSIZE(value->unitData.velocityUnits));
		break;
	case UnitType::Mass:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.massUnits, IM_ARRAYSIZE(value->unitData.massUnits));
		break;
	case UnitType::Time:
		changed = ImGui::Combo(id.c_str(), &units, value->unitData.timeUnits, IM_ARRAYSIZE(value->unitData.timeUnits));
		break;
	}
	if (changed)
		value->ConvertToUnits(units);

	return changed;
}

//imgui sliders might eventually get a flag that allows clamping automatically, but until then I need to handle it
template <typename T> T UserInterface::clip(const T& n, const T& lower, const T& upper) {
	if (n < lower)
		return lower;
	if (n > upper)
		return upper;

	return n;
}

void UserInterface::LoadPopup(Physics* physics) {
	//would be nicer if I could put OpenPopup inside the menu item, and get rid of showLoadPopup. Unfortunately, OpenPopup only works if it is on the same level as BeginPopupModal. 
	//I can't put BeginPopupModal inside the menu, because it would then only show up if the menu is open (which is not the case after you click a menu item)
	//https://github.com/ocornut/imgui/issues/331
	if (showLoadPopup)
	{
		ImGui::OpenPopup("Load Scenario");
	}

	if (ImGui::BeginPopupModal("Load Scenario"))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 12));
		std::vector<std::string> invalidFiles = {};

		for (int i = 0; i < saveFiles.size(); i++)
		{
			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load_file(("../saves/" + saveFiles[i]).c_str());

			if (result)
			{
				std::string selectableText = std::to_string(i + 1 - invalidFiles.size()) + ". " + saveFiles[i];
				//note: no & on imguiStatus->selected[i], and the flag to NOT automatically close popups. That was confusing to figure out...
				if (ImGui::Selectable(selectableText.c_str(), selected[i], ImGuiSelectableFlags_DontClosePopups))
				{
					std::fill(selected.begin(), selected.end(), false);
					selected[i] = true;
				}
			}
			else
			{
				invalidFiles.push_back(saveFiles[i] + " --- Error: " + result.description());
			}
		}

		if (!invalidFiles.empty())
		{
			if (ImGui::CollapsingHeader("Invalid Files"))
			{
				for (int i = 0; i < invalidFiles.size(); i++)
					ImGui::TextWrapped(invalidFiles[i].c_str());
			}
		}

		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			for (int i = 0; i < selected.size(); i++)
			{
				if (selected[i])
					Physics::FromXml(physics, "../saves/" + saveFiles[i], textureFolders);
			}

			showLoadPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			showLoadPopup = false;
		}
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
}

void UserInterface::TopLeftOverlay(Physics* physics)
{
	ImGui::SetNextWindowPos(ImVec2(5, 20));
	ImGui::Begin("TopLeftOverlay", NULL, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text(("Time: " + std::to_string(physics->time) + " years").c_str());
	ImGui::Text("FPS: %.3f", ImGui::GetIO().Framerate);
	ImGui::End();
}

void UserInterface::SavePopup(Physics* physics) {
	if (showSavePopup)
	{
		ImGui::OpenPopup("Save");
	}

	if (ImGui::BeginPopupModal("Save"))
	{
		static char filename[128] = "";
		ImGui::Text("Name"); ImGui::SameLine();
		ImGui::InputText("##SaveName", filename, IM_ARRAYSIZE(filename));

		if (ImGui::Button("Save##Button", ImVec2(120, 0)))
		{
			Physics::ToXml(*physics, "../saves/" + std::string(filename) + ".xml");
			saveFiles.push_back(std::string(filename) + ".xml");
			selected.push_back(false);
			ImGui::CloseCurrentPopup();
			showSavePopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			showSavePopup = false;
		}

		ImGui::EndPopup();
	}
}

void UserInterface::MenuBar(Physics* physics) {
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save As..."))
				showSavePopup = true;
			if (ImGui::MenuItem("Load"))
				showLoadPopup = true;

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Top Left Overlay", NULL, &showTopLeftOverlay);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	LoadPopup(physics);
	SavePopup(physics);
}

void UserInterface::ShowMainUi(Physics* physics, Camera* camera, float* xTranslate, float* yTranslate, float* zTranslate)
{
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_ShowBorders;

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4{ 255,255,255,255 };

	MenuBar(physics);

	if (showTopLeftOverlay)
		TopLeftOverlay(physics);

	ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("Simulation Controls", &showMainWindow, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGuiTreeNodeFlags treeFlags = 0;
	treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

	if (ImGui::CollapsingHeader("Setup", treeFlags))
	{

		//hack since imgui can't place legit labels on the left currently. It's a planned feature, so I'll fix this when that happens
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Algorithm "); ImGui::SameLine();
		ImGui::PushItemWidth(288);
		ImGui::Combo("##Algorithm", &physics->selectedAlgorithm, physics->algorithms, IM_ARRAYSIZE(physics->algorithms));
		ImGui::PopItemWidth();

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Timestep  "); ImGui::SameLine();
		ImGui::PushItemWidth(200);
		ImGui::InputFloat("##Timestep", &physics->timestep.value, 0.001f); ImGui::SameLine();
		ImGui::PushItemWidth(80);
		UnitCombo<UnitType::Time>("##TimestepUnits", &physics->timestep);
		ImGui::PopItemWidth();

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Total Time"); ImGui::SameLine();
		ImGui::PushItemWidth(200);
		ImGui::InputFloat("##Total Time", &physics->totalTime.value, 0.01f); ImGui::SameLine();
		ImGui::PushItemWidth(80);
		UnitCombo<UnitType::Time>("##TotalTimeUnits", &physics->totalTime);
		ImGui::PopItemWidth();

		int totalTimesteps = round(physics->totalTime.GetBaseValue() / physics->timestep.GetBaseValue());
		ImGui::PushItemWidth(300);
		for (int i = 0; i < physics->getCurrentObjects().size(); i++) {
			PhysObject currentObject = physics->computedData[physics->dataIndex][i];
			if (ImGui::CollapsingHeader(currentObject.name.c_str()))
			{
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::Text("Mass    "); ImGui::SameLine();
				bool massChanged = InputScientific(("##Mass" + currentObject.name).c_str(), &physics->computedData[physics->dataIndex][i].mass.value);

				//default spacing between units and entry boxes is inconsistent form some reason, so have to hardcode position on the line. Sad.
				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool massUnitsChanged = UnitCombo<UnitType::Mass>("##Mass" + currentObject.name, &physics->computedData[physics->dataIndex][i].mass);
				ImGui::PopItemWidth();

				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::Text("Position"); ImGui::SameLine();
				bool positionChanged = ImGui::InputFloat3(("##Position " + currentObject.name).c_str(), &physics->computedData[physics->dataIndex][i].position.value[0]);

				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool positionUnitsChanged = UnitCombo3<UnitType::Distance>("##PositionUnits" + currentObject.name, &physics->computedData[physics->dataIndex][i].position);
				ImGui::PopItemWidth();

				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::Text("Velocity"); ImGui::SameLine();
				bool velocityChanged = ImGui::InputFloat3(("##Velocity " + currentObject.name).c_str(), &physics->computedData[physics->dataIndex][i].velocity.value[0]);

				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool velocityUnitsChanged = UnitCombo3<UnitType::Velocity>("##VelocityUnits" + currentObject.name, &physics->computedData[physics->dataIndex][i].velocity);
				ImGui::PopItemWidth();

				if (!isPaused && (massChanged || positionChanged || velocityChanged))
					isPaused = true;

				if (massUnitsChanged || positionUnitsChanged || velocityUnitsChanged) {
					for (int j = 0; j < physics->computedData.size(); j++) {
						if (massUnitsChanged) {
							int newMassUnit = physics->computedData[physics->dataIndex][i].mass.unitIndex;
							physics->computedData[j][i].mass.ConvertToUnits(newMassUnit);
						}

						if (positionUnitsChanged) {
							int newPositionUnit = physics->computedData[physics->dataIndex][i].position.unitIndex;
							physics->computedData[j][i].position.ConvertToUnits(newPositionUnit);
						}

						if (velocityUnitsChanged) {
							int newVelocityUnit = physics->computedData[physics->dataIndex][i].velocity.unitIndex;
							physics->computedData[j][i].velocity.ConvertToUnits(newVelocityUnit);
						}
					}
				}
			}
		}

		if (ImGui::Button("Compute", ImVec2(ImGui::GetWindowContentRegionWidth(), 30)))
		{
			isPaused = true;

			physics->temporaryData = physics->computedData;

			physics->temporaryIndex = physics->dataIndex;

			std::vector<PhysObject> currentFrame = physics->getCurrentObjects();
			physics->computedData = { currentFrame };
			physics->dataIndex = 0;
			physics->updatePaths(true);

			ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiSetCond_FirstUseEver);
			ImGui::OpenPopup("Computing timesteps...");
		}
		if (ImGui::BeginPopupModal("Computing timesteps..."))
		{
			char progressString[32];

			if (physics->dataIndex + 1 > totalTimesteps)
				ImGui::CloseCurrentPopup();
			else {
				physics->step(physics->timestep.GetBaseValue());
				physics->updatePaths(false);
			}
			sprintf_s(progressString, "%d/%d", physics->dataIndex + 1, totalTimesteps);

			ImGui::ProgressBar((float)physics->dataIndex / totalTimesteps, ImVec2(0.f, 0.f), progressString);


			if (ImGui::Button("Cancel")) {
				physics->computedData = physics->temporaryData;
				physics->dataIndex = physics->temporaryIndex;
				physics->temporaryData = {};

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

	}
	if (ImGui::CollapsingHeader("Playback", treeFlags))
	{
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Object Focus  "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		std::vector<std::string> names = physics->GetObjectNames();

		//https://eliasdaler.github.io/using-imgui-with-sfml-pt2#combobox-listbox
		static auto vector_getter = [](void* vec, int idx, const char** out_text)
		{
			auto& vector = *static_cast<std::vector<std::string>*>(vec);
			if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
			*out_text = vector.at(idx).c_str();
			return true;
		};

		if (ImGui::Combo("##ObjectFocus", &physics->objectFocus, vector_getter, static_cast<void*>(&names), names.size()))
		{
			int originalIndex = physics->dataIndex;

			for (physics->dataIndex = 0; physics->dataIndex < physics->computedData.size(); physics->dataIndex++)
				physics->updatePaths(physics->dataIndex == 0);

			physics->dataIndex = originalIndex;

			//changing focus, and want to re-center on the new target
			*xTranslate = 0.0;
			*yTranslate = 0.0;
		}

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Azimuth       "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if (ImGui::SliderFloat("##azimuth", &camera->azimuth, -360, 360.0))
			camera->azimuth = clip(camera->azimuth, -360.0f, 360.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Inclination   "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if (ImGui::SliderFloat("##inclination", &camera->inclination, 0, 90.0))
			camera->inclination = clip(camera->inclination, 0.0f, 90.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Camera Zoom   "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if (ImGui::SliderFloat("##zoom", &camera->Zoom, 0, 45.0))
			camera->Zoom = clip(camera->Zoom, 0.0f, 45.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Playback Speed"); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		ImGui::InputInt("##Playback Speed", &physics->playbackSpeed);

		if (ImGui::Button(isPaused ? "Play" : "Pause", ImVec2(97, 0)))
		{
			isPaused = !isPaused;
		}
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if (ImGui::SliderInt("##playbackSlider", &physics->dataIndex, 0, physics->computedData.size() - 1))
			physics->dataIndex = clip(physics->dataIndex, 0, (int)physics->computedData.size() - 1);
	}

	ImGui::End();
}

//http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
//https://stackoverflow.com/questions/2239872/how-to-get-list-of-folders-in-this-folder
//https://msdn.microsoft.com/en-us/library/aa365200(VS.85).aspx
//windows specific
std::vector<std::string> UserInterface::GetAllFilesInFolder(std::string folderPath)
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

std::vector<std::string> UserInterface::GetAllFoldersInFolder(std::string folderPath)
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