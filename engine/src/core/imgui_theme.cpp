#include <engine/core/imgui_theme.h>

#include "imgui.h"

namespace chad
{

void imguiApplyTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // Rounded corners for a slightly modern but not flashy feel
    style.WindowRounding    = 5.3F;
    style.FrameRounding     = 2.3F;
    style.ScrollbarRounding = 0.0F;

    // Dark, desaturated palette — easy on the eyes during long debug sessions
    style.Colors[ImGuiCol_Text]                 = ImVec4(0.90F, 0.90F, 0.90F, 0.90F);
    style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.60F, 0.60F, 0.60F, 1.00F);
    style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.09F, 0.09F, 0.15F, 1.00F);
    style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
    style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.05F, 0.05F, 0.10F, 0.85F);
    style.Colors[ImGuiCol_Border]               = ImVec4(0.70F, 0.70F, 0.70F, 0.65F);
    style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
    style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.00F, 0.00F, 0.01F, 1.00F);
    style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.90F, 0.80F, 0.80F, 0.40F);
    style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.90F, 0.65F, 0.65F, 0.45F);
    style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.00F, 0.00F, 0.00F, 0.83F);
    style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.40F, 0.40F, 0.80F, 0.20F);
    style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.00F, 0.00F, 0.00F, 0.87F);
    style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.01F, 0.01F, 0.02F, 0.80F);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.20F, 0.25F, 0.30F, 0.60F);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.55F, 0.53F, 0.55F, 0.51F);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56F, 0.56F, 0.56F, 1.00F);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.56F, 0.56F, 0.56F, 0.91F);
    style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.90F, 0.90F, 0.90F, 0.83F);
    style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.70F, 0.70F, 0.70F, 0.62F);
    style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.30F, 0.30F, 0.30F, 0.84F);
    style.Colors[ImGuiCol_Button]               = ImVec4(0.48F, 0.72F, 0.89F, 0.49F);
    style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.50F, 0.69F, 0.99F, 0.68F);
    style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.80F, 0.50F, 0.50F, 1.00F);
    style.Colors[ImGuiCol_Header]               = ImVec4(0.30F, 0.69F, 1.00F, 0.53F);
    style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.44F, 0.61F, 0.86F, 1.00F);
    style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.38F, 0.62F, 0.83F, 1.00F);
    style.Colors[ImGuiCol_Separator]            = ImVec4(0.50F, 0.50F, 0.50F, 1.00F);
    style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.70F, 0.60F, 0.60F, 1.00F);
    style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.90F, 0.70F, 0.70F, 1.00F);
    style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00F, 1.00F, 1.00F, 0.85F);
    style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(1.00F, 1.00F, 1.00F, 0.60F);
    style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(1.00F, 1.00F, 1.00F, 0.90F);
    style.Colors[ImGuiCol_PlotLines]            = ImVec4(1.00F, 1.00F, 1.00F, 1.00F);
    style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.90F, 0.70F, 0.00F, 1.00F);
    style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90F, 0.70F, 0.00F, 1.00F);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00F, 0.60F, 0.00F, 1.00F);
    style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.00F, 0.00F, 1.00F, 0.35F);
    style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.20F, 0.20F, 0.20F, 0.35F);
}

}  // namespace chad
