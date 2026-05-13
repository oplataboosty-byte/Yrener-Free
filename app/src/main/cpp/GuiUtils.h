#pragma once
#include "wLibs/ImGui/imgui.h"
#include "wLibs/ImGui/imgui_internal.h"
#include "MenuConfig.h"
#include <cmath>


static float g_HoverAnimations[512] = {0};
static int g_AnimIndex = 0;


void ApplyTheme(int themeIndex) {
    ImGuiStyle& style = ImGui::GetStyle();
    ThemeColors tc = GetThemeColors(themeIndex);

    style.WindowRounding = 18.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 10.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 12.0f;
    style.ItemSpacing = ImVec2(12, 14);
    style.TouchExtraPadding = ImVec2(10, 10);
    style.WindowBorderSize = 1.5f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(18, 18);
    style.FramePadding = ImVec2(14, 10);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(tc.windowBg.x, tc.windowBg.y, tc.windowBg.z, g_Menu.menuAlpha);
    colors[ImGuiCol_ChildBg] = ImVec4(tc.windowBg.x * 0.8f, tc.windowBg.y * 0.8f, tc.windowBg.z * 0.8f, 0.6f);
    colors[ImGuiCol_Text] = tc.text;
    colors[ImGuiCol_TextDisabled] = tc.textDim;
    colors[ImGuiCol_Button] = tc.accent;
    colors[ImGuiCol_ButtonHovered] = tc.accentHover;
    colors[ImGuiCol_ButtonActive] = tc.accentActive;
    colors[ImGuiCol_FrameBg] = tc.frameBg;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(tc.frameBg.x + 0.05f, tc.frameBg.y + 0.05f, tc.frameBg.z + 0.05f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(tc.frameBg.x + 0.1f, tc.frameBg.y + 0.1f, tc.frameBg.z + 0.1f, 1.0f);
    colors[ImGuiCol_CheckMark] = tc.accentActive;
    colors[ImGuiCol_SliderGrab] = tc.accent;
    colors[ImGuiCol_SliderGrabActive] = tc.accentActive;
    colors[ImGuiCol_Border] = tc.border;
    colors[ImGuiCol_Separator] = tc.border;
    colors[ImGuiCol_Header] = tc.accent;
    colors[ImGuiCol_HeaderHovered] = tc.accentHover;
    colors[ImGuiCol_HeaderActive] = tc.accentActive;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(tc.frameBg.x, tc.frameBg.y, tc.frameBg.z, 0.5f);
    colors[ImGuiCol_ScrollbarGrab] = tc.accent;
    colors[ImGuiCol_ScrollbarGrabHovered] = tc.accentHover;
    colors[ImGuiCol_ScrollbarGrabActive] = tc.accentActive;
}


