/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "VRPlayer_preview_window_ui.h"
#include "VRPlayer_settings.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

PreviewWindowUI::PreviewWindowUI(Settings*          in_settings_ptr,
                                 const IVRPlayback* in_vr_playback_ptr)
    :m_imgui_context_ptr(nullptr),
     m_settings_ptr     (in_settings_ptr),
     m_vr_playback_ptr  (in_vr_playback_ptr)
{
    /* Stub */
}

PreviewWindowUI::~PreviewWindowUI()
{
    /* Stub */
}

PreviewWindowUIUniquePtr PreviewWindowUI::create(Settings*          in_settings_ptr,
                                                 const IVRPlayback* in_vr_playback_ptr)
{
    PreviewWindowUIUniquePtr result_ptr(new PreviewWindowUI(in_settings_ptr,
                                                            in_vr_playback_ptr) );

    AI_ASSERT(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

uint32_t PreviewWindowUI::get_height_px() const
{
    const auto show_ui_separation_slider    = !m_vr_playback_ptr->supports_separate_ui_texture    ();
    const auto show_viewport_offset_sliders =  m_vr_playback_ptr->needs_manual_viewport_adjustment();

    uint32_t result = 60;

    if (show_ui_separation_slider)
    {
        result += 80;
    }

    if (show_viewport_offset_sliders)
    {
        result += 40;
    }

    return result;
}

bool PreviewWindowUI::init()
{
    IMGUI_CHECKVERSION();

    m_imgui_context_ptr = ImGui::CreateContext();
    AI_ASSERT(m_imgui_context_ptr != nullptr);

    ImGui::StyleColorsLight();

    return true;
}

void PreviewWindowUI::render(const uint32_t& in_start_y,
                             const uint32_t& in_width)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame   ();
    ImGui::NewFrame           ();

    ImGui::Begin("Hello.",
                 nullptr, /* p_open */
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
               const auto  show_ui_separation_slider    = !m_vr_playback_ptr->supports_separate_ui_texture    ();
               const auto  show_viewport_offset_sliders =  m_vr_playback_ptr->needs_manual_viewport_adjustment();
        static const float text_to_control_ratio        = 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,
                            1.0f);

        ImGui::SetWindowPos (ImVec2(0.0f,                         static_cast<float>(in_start_y)      ));
        ImGui::SetWindowSize(ImVec2(static_cast<float>(in_width), static_cast<float>(get_height_px() )));

        if (show_ui_separation_slider)
        {
            ImGui::LabelText       ("##",
                                    "Console window Y offset");
            ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderInt       ("##ConsoleWindowYOffset",
                                    m_settings_ptr->get_console_window_y_offset_ptr(),
                                    0,    /* v_min */
                                    1000, /* v_max */
                                    "%d",
                                    ImGuiSliderFlags_NoInput);
        }

        ImGui::LabelText       ("##",
                                "Eye separation multiplier");
        ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat     ("##EyeSep",
                                m_settings_ptr->get_eye_separation_multiplier_ptr(),
                                0.f,   /* v_min */
                                1.0f,  /* v_max */
                                "%.3f",
                                ImGuiSliderFlags_NoInput);

        if (show_ui_separation_slider)
        {
            ImGui::LabelText       ("##",
                                    "UI separation multiplier");
            ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat     ("##OrthoSep",
                                    m_settings_ptr->get_ortho_separation_multiplier_ptr(),
                                    0.001f, /* v_min */
                                    1.0f,   /* v_max */
                                    "%.3f",
                                    ImGuiSliderFlags_NoInput);

            ImGui::LabelText       ("##",
                                    "Status bar Y offset");
            ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderInt       ("##StatusBarYOffset",
                                    m_settings_ptr->get_status_bar_y_offset_ptr(),
                                    0,    /* v_min */
                                    1000, /* v_max */
                                    "%d",
                                    ImGuiSliderFlags_NoInput);

            ImGui::LabelText  ("##",
                               "UI scale");
            ImGui::SameLine   (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##UIScale",
                               m_settings_ptr->get_ui_scale_ptr(),
                               0.001f,  /* v_min */
                               0.9f,    /* v_max */
                               "%.3f",
                               ImGuiSliderFlags_NoInput);
        }

        if (show_viewport_offset_sliders)
        {
            ImGui::LabelText       ("##",
                                    "Viewport offset multiplier (X)");
            ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat     ("##ViewportOffsetMulX",
                                    m_settings_ptr->get_viewport_offset_x_multiplier_ptr(),
                                    0.075f,  /* v_min */
                                    0.125f,  /* v_max */
                                    "%.3f",
                                    ImGuiSliderFlags_NoInput);

            ImGui::LabelText       ("##",
                                    "Viewport offset multiplier (Y)");
            ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat     ("##ViewportOffsetMulY",
                                    m_settings_ptr->get_viewport_offset_y_multiplier_ptr(),
                                    0.05f,  /* v_min */
                                    0.1f,   /* v_max */
                                    "%.3f",
                                    ImGuiSliderFlags_NoInput);
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();

    ImGui::Render                   ();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );
}

void PreviewWindowUI::setup_for_bound_context(GLFWwindow* in_glfw_preview_window_ptr)
{
    ImGui::SetCurrentContext(m_imgui_context_ptr);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(in_glfw_preview_window_ptr,
                                 true); /* install_callbacks */
    ImGui_ImplOpenGL3_Init      ("#version 100");
}