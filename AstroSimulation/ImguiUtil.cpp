// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "ImguiUtil.h"

#include <Windows.h>

// GL3W/GLFW
#include <GL/glew.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "Simulation.h"
#include "Physics.h"
#include "PhysObject.h"
#include "ObjectSettings.h"

// Data
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGlfwGL3_RenderDrawLists(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glActiveTexture(last_active_texture);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

static const char* ImGui_ImplGlfwGL3_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfwGL3_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MousePressed[button] = true;
}

void ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_ImplGlfwGL3_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfwGL3_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfwGL3_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

bool ImGui_ImplGlfwGL3_CreateDeviceObjects()
{
    // Backup GL state
    GLint last_texture, last_array_buffer, last_vertex_array;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    g_ShaderHandle = glCreateProgram();
    g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    glCompileShader(g_VertHandle);
    glCompileShader(g_FragHandle);
    glAttachShader(g_ShaderHandle, g_VertHandle);
    glAttachShader(g_ShaderHandle, g_FragHandle);
    glLinkProgram(g_ShaderHandle);

    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_ElementsHandle);

    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

    ImGui_ImplGlfwGL3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    return true;
}

void    ImGui_ImplGlfwGL3_InvalidateDeviceObjects()
{
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

    if (g_ShaderHandle && g_VertHandle) glDetachShader(g_ShaderHandle, g_VertHandle);
    if (g_VertHandle) glDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    if (g_ShaderHandle && g_FragHandle) glDetachShader(g_ShaderHandle, g_FragHandle);
    if (g_FragHandle) glDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool    ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks)
{
    g_Window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImGui_ImplGlfwGL3_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;
    io.ClipboardUserData = g_Window;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlfwGL3_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
    }

    return true;
}

void ImGui_ImplGlfwGL3_Shutdown()
{
    ImGui_ImplGlfwGL3_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGlfwGL3_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplGlfwGL3_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

bool DataTypeApplyOpFromTextScientific(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* scalar_format)
{
	scalar_format = "%f";
	float* v = (float*)data_ptr;
	const float old_v = *v;
	*v = (float)atof(buf);
	return *v != old_v;
}

bool InputScalarScientific(const char* label, ImGuiDataType data_type, void* data_ptr, const char* scalar_format, ImGuiInputTextFlags extra_flags)
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
bool InputScientific(const char* label, float* v, const char *display_format, ImGuiInputTextFlags extra_flags)
{
	return InputScalarScientific(label, ImGuiDataType_Float, (void*)v, display_format, extra_flags);
}

//imgui sliders might eventually get a flag that allows clamping automatically, but until then I need to handle it
template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
	if (n < lower)
		return lower;
	if (n > upper)
		return upper;

	return n;
}

template <UnitType type>
bool UnitCombo(std::string id, ValueWithUnits<type>* value) {
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

template <UnitType type>
bool UnitCombo3(std::string id, ValueWithUnits3<type>* value) {
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

// Demonstrate creating a simple static window with no decoration.
void TopLeftOverlay(Physics* physics)
{
	ImGui::SetNextWindowPos(ImVec2(5, 20));
	ImGui::Begin("TopLeftOverlay", NULL, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text(("Time: " + std::to_string(physics->time)).c_str());
	ImGui::Text("FPS: %.3f", ImGui::GetIO().Framerate);
	ImGui::End();
}

void LoadPopup(Physics* physics, ImguiStatus* imguiStatus) {
	//would be nicer if I could put OpenPopup inside the menu item, and get rid of showLoadPopup. Unfortunately, OpenPopup only works if it is on the same level as BeginPopupModal. 
	//I can't put BeginPopupModal inside the menu, because it would then only show up if the menu is open (which is not the case after you click a menu item)
	//https://github.com/ocornut/imgui/issues/331
	if (imguiStatus->showLoadPopup)
	{
		ImGui::OpenPopup("Load Scenario");
	}

	if (ImGui::BeginPopupModal("Load Scenario"))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 12));
		std::vector<std::string> invalidFiles = {};

		for (int i = 0; i < imguiStatus->saveFiles.size(); i++)
		{
			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load_file(("../saves/" + imguiStatus->saveFiles[i]).c_str());

			if (result)
			{
				std::string selectableText = std::to_string(i + 1 - invalidFiles.size()) + ". " + imguiStatus->saveFiles[i];
				//note: no & on imguiStatus->selected[i], and the flag to NOT automatically close popups. That was confusing to figure out...
				if (ImGui::Selectable(selectableText.c_str(), imguiStatus->selected[i], ImGuiSelectableFlags_DontClosePopups))
				{
					std::fill(imguiStatus->selected.begin(), imguiStatus->selected.end(), false);
					imguiStatus->selected[i] = true;
				}
			}
			else
			{
				invalidFiles.push_back(imguiStatus->saveFiles[i] + " --- Error: " + result.description());
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
			for (int i = 0; i < imguiStatus->selected.size(); i++)
			{
				if (imguiStatus->selected[i])
					Physics::FromXml(physics, "../saves/" + imguiStatus->saveFiles[i], imguiStatus->textureFolders);
			}

			imguiStatus->showLoadPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			imguiStatus->showLoadPopup = false;
		}
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
}

void SavePopup(Physics* physics, ImguiStatus* imguiStatus) {
	if (imguiStatus->showSavePopup)
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
			imguiStatus->saveFiles.push_back(std::string(filename) + ".xml");
			imguiStatus->selected.push_back(false);
			ImGui::CloseCurrentPopup();
			imguiStatus->showSavePopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			imguiStatus->showSavePopup = false;
		}

		ImGui::EndPopup();
	}
}

void MenuBar(Physics* physics, ImguiStatus* imguiStatus) {
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save As..."))
				imguiStatus->showSavePopup = true;
			if (ImGui::MenuItem("Load"))
				imguiStatus->showLoadPopup = true;

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Top Left Overlay", NULL, &imguiStatus->showTopLeftOverlay);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	LoadPopup(physics, imguiStatus);
	SavePopup(physics, imguiStatus);
}

void ShowMainUi(Physics* physics, std::vector<std::vector<GLfloat> > * paths, Camera* camera, ImguiStatus* imguiStatus, float* xTranslate, float* yTranslate, float* zTranslate)
{
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_ShowBorders;

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4{ 255,255,255,255 };

	MenuBar(physics, imguiStatus);

	if (imguiStatus->showTopLeftOverlay)
		TopLeftOverlay(physics);

	ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("Simulation Controls", &imguiStatus->showMainWindow, window_flags))
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
				bool massUnitsChanged = UnitCombo<UnitType::Mass>("##Mass" + currentObject.name,&physics->computedData[physics->dataIndex][i].mass);
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

				if (!imguiStatus->isPaused && (massChanged || positionChanged || velocityChanged))
					imguiStatus->isPaused = true;

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
			imguiStatus->isPaused = true;

			physics->temporaryData = physics->computedData;

			physics->temporaryIndex = physics->dataIndex;

			std::vector<PhysObject> currentFrame = physics->getCurrentObjects();
			physics->computedData = { currentFrame };
			physics->dataIndex = 0;
			Simulation::updatePaths(physics, paths, true);

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
				Simulation::updatePaths(physics, paths, false);
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

		if (ImGui::Combo("##ObjectFocus", &physics->objectFocus, vector_getter, static_cast<void*>(&names), names.size())) 
		{
			int originalIndex = physics->dataIndex;

			for (physics->dataIndex = 0; physics->dataIndex < physics->computedData.size(); physics->dataIndex++)
				Simulation::updatePaths(physics, paths, physics->dataIndex == 0);

			physics->dataIndex = originalIndex;

			//changing focus, and want to re-center on the new target
			*xTranslate = 0.0;
			*yTranslate = 0.0;
		}

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Azimuth       "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if(ImGui::SliderFloat("##azimuth", &camera->azimuth, -360, 360.0))
			camera->azimuth = clip(camera->azimuth, -360.0f, 360.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Inclination   "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if(ImGui::SliderFloat("##inclination", &camera->inclination, 0, 90.0))
			camera->inclination = clip(camera->inclination, 0.0f, 90.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Camera Zoom   "); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if(ImGui::SliderFloat("##zoom", &camera->Zoom, 0, 45.0))
			camera->Zoom = clip(camera->Zoom, 0.0f, 45.0f);

		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Playback Speed"); ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		ImGui::InputInt("##Playback Speed", &physics->playbackSpeed);

		if (ImGui::Button(imguiStatus->isPaused ? "Play" : "Pause", ImVec2(97, 0)))
		{
			imguiStatus->isPaused = !(imguiStatus->isPaused);
		}
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		if(ImGui::SliderInt("##playbackSlider", &physics->dataIndex, 0, physics->computedData.size() - 1))
			physics->dataIndex = clip(physics->dataIndex, 0, (int)physics->computedData.size() - 1);
	}

	ImGui::End();
}