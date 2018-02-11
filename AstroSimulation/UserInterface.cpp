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

	WindowFlags = 0;
	WindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

	TreeNodeFlags = 0;
	TreeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

	style = Style::Dark;
	UpdateStyle();
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
	if (ImGui::InputText(label, buf, IM_ARRAYSIZE(buf), extra_flags))
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
	if (ShowLoadPopup)
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

			ShowLoadPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			ShowLoadPopup = false;
		}
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
}

void UserInterface::TopLeftOverlay(Physics* physics)
{
	ImGui::SetNextWindowPos(ImVec2(5, 24));
	ImGui::Begin("TopLeftOverlay", NULL, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text(("Time: " + std::to_string(physics->time) + " years").c_str());
	ImGui::Text("FPS: %.3f", ImGui::GetIO().Framerate);
	ImGui::End();
}

void UserInterface::SavePopup(Physics* physics) {
	if (ShowSavePopup)
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
			ShowSavePopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			ShowSavePopup = false;
		}

		ImGui::EndPopup();
	}
}

void UserInterface::UpdateStyle()
{
	switch (style) {
	case Style::Classic:
		ImGui::StyleColorsClassic();
		break;
	case Style::Light:
		ImGui::StyleColorsLight();
		break;
	case Style::Dark:
		ImGui::StyleColorsDark();
		break;
	}

	ImGuiStyle* customStyle = &ImGui::GetStyle();
	customStyle->WindowBorderSize = 1.0f;
}

void UserInterface::MenuBar(Physics* physics) {
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ShowSavePopup = ImGui::MenuItem("Save As...");
			ShowLoadPopup = ImGui::MenuItem("Load");

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::BeginMenu("Style"))
			{
				if(ImGui::MenuItem("Classic", NULL, style == Style::Classic)) {
					style = Style::Classic;
					UpdateStyle();
				}
				if (ImGui::MenuItem("Dark", NULL, style == Style::Dark)) {
					style = Style::Dark;
					UpdateStyle();
				}
				if (ImGui::MenuItem("Light", NULL, style == Style::Light)) {
					style = Style::Light;
					UpdateStyle();
				}

				ImGui::EndMenu();
			}

			ImGui::MenuItem("Simulation Controls", NULL, &ShowSimulationWindow);
			ImGui::MenuItem("Camera", NULL, &ShowCameraWindow);
			ImGui::MenuItem("Top Left Overlay", NULL, &ShowTopLeftOverlay);

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	LoadPopup(physics);
	SavePopup(physics);
}

void UserInterface::ShowMainUi(Physics* physics, Graphics * graphics)
{
	MenuBar(physics);

	if(ShowTopLeftOverlay)
		TopLeftOverlay(physics);

	ObjectDataWindows(physics);

	if(ShowCameraWindow)
		CameraWindow(&graphics->camera);

	if(ShowSimulationWindow)
		SimulationWindow(physics, graphics);
}

void UserInterface::OriginDropdown(Physics * physics, Graphics * graphics)
{
	std::vector<std::string> names = physics->GetObjectNames();

	//https://eliasdaler.github.io/using-imgui-with-sfml-pt2#combobox-listbox
	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	if (ImGui::Combo("##ObjectFocus", &physics->origin, vector_getter, static_cast<void*>(&names), names.size()))
	{
		int originalIndex = physics->dataIndex;

		for (physics->dataIndex = 0; physics->dataIndex < physics->computedData.size(); physics->dataIndex++)
			physics->updatePaths(physics->dataIndex == 0);

		physics->dataIndex = originalIndex;

		//changing focus, and want to re-center on the new target
		graphics->xTranslate = 0.0;
		graphics->yTranslate = 0.0;
	}
}

void UserInterface::SimulationWindow(Physics * physics, Graphics * graphics)
{
	if (ImGui::Begin("Simulation Controls", &ShowSimulationWindow, WindowFlags))
	{
		if (ImGui::CollapsingHeader("Setup", TreeNodeFlags))
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

			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Origin    "); ImGui::SameLine();
			ImGui::PushItemWidth(288);
			OriginDropdown(physics, graphics);

			ImGui::PushItemWidth(250);

			std::vector<PhysObject> objects = physics->getCurrentObjects();
			std::list<PhysObject> objectsList(objects.begin(), objects.end());
			ObjectsTree(objectsList, graphics);

			if (ImGui::Button("Compute", ImVec2(ImGui::GetWindowContentRegionWidth(), 30)))
			{
				isPaused = true;

				physics->temporaryData = physics->computedData;

				physics->temporaryIndex = physics->dataIndex;

				physics->computedData = { objects };
				physics->dataIndex = 0;
				physics->updatePaths(true);

				ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiSetCond_FirstUseEver);
				ImGui::OpenPopup("Computing timesteps...");
			}
			if (ImGui::BeginPopupModal("Computing timesteps..."))
			{
				char progressString[32];

				int totalTimesteps = round(physics->totalTime.GetBaseValue() / physics->timestep.GetBaseValue());
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
		if (ImGui::CollapsingHeader("Playback", TreeNodeFlags))
		{
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Playback Speed"); ImGui::SameLine();
			ImGui::InputInt("##Playback Speed", &physics->playbackSpeed);

			if (ImGui::Button(isPaused ? "Play" : "Pause", ImVec2(97, 0)))
			{
				isPaused = !isPaused;
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##playbackSlider", &physics->dataIndex, 0, physics->computedData.size() - 1))
				physics->dataIndex = clip(physics->dataIndex, 0, (int)physics->computedData.size() - 1);
		}
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

void UserInterface::ObjectsTree(std::list<PhysObject> objects, Graphics * graphics)
{
	std::list<PhysObject>::iterator objectItr = objects.begin();
	while (objects.size() > 0 && objectItr != objects.end()) 
	{
		std::list<PhysObject> satelliteObjects = {};

		//find any satellites of current object, and remove them from the toplevel list of objects so they don't get added twice
		if (objectItr->satellites.size() > 0) 
		{
			std::vector<std::string>::const_iterator satelliteItr = objectItr->satellites.begin();

			for (std::vector<std::string>::const_iterator satelliteItr = objectItr->satellites.begin(); satelliteItr != objectItr->satellites.end(); satelliteItr++)
			{

				std::string satelliteName = *satelliteItr;
				//https://stackoverflow.com/questions/15517991/search-a-vector-of-objects-by-object-attribute
				auto matchItr = std::find_if(objects.begin(), objects.end(), [&satelliteName](const PhysObject& obj) {return obj.name == satelliteName; });
				if (matchItr != objects.end())
				{
					satelliteObjects.push_back(*matchItr);
					if (std::distance(objects.begin(), matchItr) < std::distance(objects.begin(), objectItr)) 
					{
						objects.erase(matchItr);
						objectItr++;
					}
					else
					{
						objects.erase(matchItr);
					}
				}
			}
		}

		ObjectsTreeNode(objectItr->name, satelliteObjects, graphics);
		objectItr++;
	}
}

void UserInterface::ObjectDataWindows(Physics * physics)
{

	std::vector<PhysObject> objects = physics->getCurrentObjects();
	for (int i = 0; i < objects.size(); i++)
	{
		std::string name = objects[i].name;
		std::string windowId = objects[i].name + "##DataWindow";
		if (ShowDataWindow[windowId])
		{
			if (ImGui::Begin(windowId.c_str(), &(ShowDataWindow[windowId]), WindowFlags))
			{
				ImGui::AlignFirstTextHeightToWidgets();

				//width for the data inputs
				ImGui::PushItemWidth(300);

				ImGui::Text("Mass    "); ImGui::SameLine();
				bool massChanged = InputScientific(("##Mass" + name).c_str(), &physics->computedData[physics->dataIndex][i].mass.value);

				//default spacing between units and entry boxes is inconsistent form some reason, so have to hardcode position on the line. Sad.
				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool massUnitsChanged = UnitCombo<UnitType::Mass>("##Mass" + name, &physics->computedData[physics->dataIndex][i].mass);
				ImGui::PopItemWidth();

				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::Text("Position"); ImGui::SameLine();
				bool positionChanged = ImGui::InputFloat3(("##Position " + name).c_str(), &physics->computedData[physics->dataIndex][i].position.value[0]);

				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool positionUnitsChanged = UnitCombo3<UnitType::Distance>("##PositionUnits" + name, &physics->computedData[physics->dataIndex][i].position);
				ImGui::PopItemWidth();

				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::Text("Velocity"); ImGui::SameLine();
				bool velocityChanged = ImGui::InputFloat3(("##Velocity " + name).c_str(), &physics->computedData[physics->dataIndex][i].velocity.value[0]);

				ImGui::SameLine(375.0f); ImGui::PushItemWidth(120);
				bool velocityUnitsChanged = UnitCombo3<UnitType::Velocity>("##VelocityUnits" + name, &physics->computedData[physics->dataIndex][i].velocity);
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
			ImGui::End();
		}
	}
}

void UserInterface::InitObjectDataWindows(std::vector<PhysObject> objects)
{
	for (int i = 0; i < objects.size(); i++)
	{
		ShowDataWindow.insert(std::pair<std::string, bool>(objects[i].name + "##DataWindow", false));
	}
}

void UserInterface::ObjectsTreeNode(std::string name, std::list<PhysObject> satelliteObjects, Graphics * graphics)
{
	if (ImGui::TreeNode(name.c_str()))
	{
		if (ImGui::Button("Data", ImVec2(50, 0)))
		{
			ShowDataWindow[name + "##DataWindow"] = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("Follow", ImVec2(50, 0)))
		{
			//do nothing for now, until I do an overhaul of my camera system.
			//can't get the object to be fully centered at high zoom. Probably a precision issue.
			//if (graphics->followObject == name)
			//	graphics->followObject = "";
			//else
			//	graphics->followObject = name;
		}

		if (satelliteObjects.size() > 0)
		{
			ObjectsTree(satelliteObjects, graphics);
		}

		ImGui::TreePop();
	}
}

void UserInterface::CameraWindow(Camera * camera)
{
	if (ImGui::Begin("Camera", &ShowCameraWindow, WindowFlags))
	{
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Azimuth    "); ImGui::SameLine();
		ImGui::PushItemWidth(250);
		if (ImGui::SliderFloat("##azimuth", &camera->azimuth, -360, 360.0))
			camera->azimuth = clip(camera->azimuth, -360.0f, 360.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Inclination"); ImGui::SameLine();
		if (ImGui::SliderFloat("##inclination", &camera->inclination, 0, 90.0))
			camera->inclination = clip(camera->inclination, 0.0f, 90.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Camera Zoom"); ImGui::SameLine();
		if (ImGui::SliderFloat("##zoom", &camera->Zoom, 0, 45.0))
			camera->Zoom = clip(camera->Zoom, 0.0f, 45.0f);

	}		
	ImGui::End();
}