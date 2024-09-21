/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_misc.h"
#include "Launcher_state.h"
#include "Launcher_ui.h"
#include "Launcher_vr_support.h"
#include <assert.h>
#include <Windows.h>

#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#define WINDOW_HEIGHT (240)
#define WINDOW_WIDTH  (320)


Launcher::UI::UI(State*     in_state_ptr,
                 VRSupport* in_vr_support_ptr)
    :m_glfw_window_ptr       (nullptr),
     m_imgui_context_ptr     (nullptr),
     m_n_selected_vr_device  (0),
     m_n_selected_vr_runtime (0),
     m_state_ptr             (in_state_ptr),
     m_vr_support_ptr        (in_vr_support_ptr),
     m_worker_thread_must_die(false)
{
    /* Stub */
}

Launcher::UI::~UI()
{
    if (m_worker_thread.joinable() )
    {
        /* Let the worker thread we want it done with. */
        m_worker_thread_must_die = true;

        /* Wait till it goes away */
        m_worker_thread.join();
    }
}

Launcher::UIUniquePtr Launcher::UI::create(State*     in_state_ptr,
                                           VRSupport* in_vr_support_ptr)
{
    Launcher::UIUniquePtr result_ptr;

    result_ptr.reset(new UI(in_state_ptr,
                            in_vr_support_ptr) );

    assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void Launcher::UI::execute()
{
    if (!glfwInit() )
    {
        assert(false);

        goto end;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_DECORATED,             GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS,            32);
    glfwWindowHint(GLFW_DOUBLEBUFFER,          1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_VISIBLE,               GLFW_TRUE);

    // Create window with graphics context
    m_glfw_window_ptr = glfwCreateWindow(WINDOW_WIDTH,
                                         WINDOW_HEIGHT,
                                         "Q1 VR launcher settings",
                                         nullptr,  /* monitor */
                                         nullptr); /* share   */

    if (m_glfw_window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    // Center the window on active screen
    {
        const auto screen_width  = ::GetSystemMetrics(SM_CXSCREEN);
        const auto screen_height = ::GetSystemMetrics(SM_CYSCREEN);

        glfwSetWindowPos(m_glfw_window_ptr,
                         (screen_width  - WINDOW_WIDTH)  / 2,
                         (screen_height - WINDOW_HEIGHT) / 2);
    }

    glfwMakeContextCurrent(m_glfw_window_ptr);

    // Set up imgui
    {
        IMGUI_CHECKVERSION();

        m_imgui_context_ptr = ImGui::CreateContext();
        assert(m_imgui_context_ptr != nullptr);

        ImGui_ImplGlfw_InitForOpenGL(m_glfw_window_ptr,
                                     true); /* install_callbacks */
        ImGui_ImplOpenGL3_Init      ("#version 100");
    }

    // Enter the window loop.
    while (!glfwWindowShouldClose(m_glfw_window_ptr) &&
           !m_worker_thread_must_die)
    {
        ::glfwPollEvents();

        // Render UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame   ();
        ImGui::NewFrame           ();

        render();

        ImGui::Render                   ();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );

        /* All done */
        glfwSwapBuffers(m_glfw_window_ptr);
    }

end:
    if (m_glfw_window_ptr != nullptr)
    {
        glfwDestroyWindow(m_glfw_window_ptr);

        m_glfw_window_ptr = nullptr;
    }

    glfwTerminate();

    /* Update state instance to contain values selected by the user. */
    if (m_available_vr_runtime_name_ptr_vec.size() > 0)
    {
        m_state_ptr->set_active_vr_backend(Common::Misc::get_vr_backend_for_text_string(m_available_vr_runtime_name_ptr_vec.at(m_n_selected_vr_runtime) ) );
    }

    if (m_specified_glquake_file_path.size() > 0)
    {
        m_state_ptr->set_glquake_exe_file_path(m_specified_glquake_file_path);
    }

    // todo: store VR device selection
}

void Launcher::UI::handle_set_glquake_location_button_click_event()
{
    std::wstring glquake_file_path;
    std::wstring glquake_file_path_with_name;

    if (show_glquake_exe_path_dialog(&glquake_file_path,
                                     &glquake_file_path_with_name) )
    {
        std::string error_string;

        if (!Launcher::Misc::check_glquake_exe_compatibility(nullptr,
                                                             glquake_file_path_with_name.c_str(),
                                                            &error_string) )
        {
            ::MessageBox(HWND_DESKTOP,
                         error_string.c_str(),
                         "Error",
                         MB_OK | MB_ICONERROR);
        }
        else
        {
            m_specified_glquake_file_path = glquake_file_path;
        }
    }
}

bool Launcher::UI::init()
{
    /* Initialize settings.. */
    const auto active_vr_backend = m_state_ptr->get_active_vr_backend();

    static const char* lib_ovr_runtime_name_ptr = "LibOVR";
    static const char* openxr_runtime_name_ptr  = "OpenXR";

    if (m_state_ptr->get_glquake_exe_file_path_ptr() != nullptr)
    {
        m_specified_glquake_file_path = *m_state_ptr->get_glquake_exe_file_path_ptr();
    }

    if (m_vr_support_ptr->is_vr_backend_supported(Common::VRBackend::LIBOVR) )
    {
        m_available_vr_runtime_name_ptr_vec.emplace_back(lib_ovr_runtime_name_ptr);

        if (active_vr_backend == Common::VRBackend::LIBOVR)
        {
            m_n_selected_vr_runtime = static_cast<uint32_t>(m_available_vr_runtime_name_ptr_vec.size() - 1);
        }
    }

    if (m_vr_support_ptr->is_vr_backend_supported(Common::VRBackend::OPENXR) )
    {
        m_available_vr_runtime_name_ptr_vec.emplace_back(openxr_runtime_name_ptr);

        if (active_vr_backend == Common::VRBackend::OPENXR)
        {
            m_n_selected_vr_runtime = static_cast<uint32_t>(m_available_vr_runtime_name_ptr_vec.size() - 1);
        }
    }

    if (m_available_vr_runtime_name_ptr_vec.size() != 0)
    {
        update_vr_device_name_vec();
    }

    /* Spawn a new window thread. */
    m_worker_thread = std::thread(&UI::execute,
                                   this);

    return true;
}

void Launcher::UI::render()
{
    ImGui::Begin("Hello.",
                 nullptr, /* p_open */
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos (ImVec2(0,            0)             );
        ImGui::SetWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT) );

        {
            static const char* label_text_ptr        = "Q1 VR Player Configuration";
            static const float text_to_control_ratio = 0.5f;

            ImGui::BeginGroup();
            {
                ImGui::SameLine( (WINDOW_WIDTH - ImGui::CalcTextSize(label_text_ptr).x) / 2, 0);
                ImGui::Text    (label_text_ptr);

                ImGui::NewLine();

                {
                    ImGui::Text            ("GLQuake directory:");
                    ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                    if (m_specified_glquake_file_path.size() == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button,
                                              ImVec4(1.0f, 0.0f, 0.0f, 1.0f) );

                        if (ImGui::Button("Click to set..##GLQuakeLocation",
                            ImVec2        (ImGui::GetContentRegionAvail().x, 0) ))
                        {
                            handle_set_glquake_location_button_click_event();
                        }
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button,
                                              ImVec4(0.0f, .25f, 0.0f, 1.0f) );

                        if (ImGui::Button("Configured!##GLQuakeLocation",
                                          ImVec2(ImGui::GetContentRegionAvail().x, 0) ))
                        {
                            handle_set_glquake_location_button_click_event();
                        }
                    }

                    ImGui::PopStyleColor();
                }

                {
                    ImGui::Text("VR runtime to use:");
                    ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                    if (m_available_vr_runtime_name_ptr_vec.size() > 0)
                    {
                        if (ImGui::Combo("##VRRuntime",
                                        &m_n_selected_vr_runtime,
                                         reinterpret_cast<const char**>(m_available_vr_runtime_name_ptr_vec.data()),
                                         static_cast     <int>         (m_available_vr_runtime_name_ptr_vec.size() )))
                        {
                            const auto selected_vr_backend = Common::Misc::get_vr_backend_for_text_string(m_available_vr_runtime_name_ptr_vec.at(m_n_selected_vr_runtime) );

                            m_state_ptr->set_active_vr_backend(selected_vr_backend);
                            update_vr_device_name_vec         ();

                            m_n_selected_vr_device = 0;
                        }
                    }
                }

                {
                    ImGui::Text("VR device to use:");
                    ImGui::SameLine        (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                    if (m_available_vr_device_name_ptr_vec.size() > 0)
                    {
                        ImGui::Combo("##VRDevice",
                                    &m_n_selected_vr_device,
                                     reinterpret_cast<const char**>(m_available_vr_device_name_ptr_vec.data()),
                                     static_cast     <int>         (m_available_vr_device_name_ptr_vec.size() ));
                    }
                }
            }
            ImGui::EndGroup();

            ImGui::SetCursorPosY(WINDOW_HEIGHT / 2);
            ImGui::Separator();

            {
                std::array<uint32_t, 2> eye_texture_extents;
                const auto              selected_vr_backend = Common::Misc::get_vr_backend_for_text_string(m_available_vr_runtime_name_ptr_vec.at(m_n_selected_vr_runtime) );
                uint32_t                refresh_rate        = 0;

                m_vr_support_ptr->get_eye_texture_extents(selected_vr_backend,
                                                         &eye_texture_extents);
                m_vr_support_ptr->get_refresh_rate       (selected_vr_backend,
                                                         &refresh_rate);

                ImGui::Text          ("Per-eye viewport:");
                ImGui::SameLine      (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      ImVec4(0.0f, 1.0f, 1.0f, 1.0f) );
                ImGui::Text          ("%d x %d",
                                      eye_texture_extents.at(0),
                                      eye_texture_extents.at(1) );
                ImGui::PopStyleColor();

                {
                    ImGui::Text          ("Refresh rate:");
                    ImGui::SameLine      (ImGui::GetContentRegionAvail().x * text_to_control_ratio);
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.0f, 1.0f, 1.0f, 1.0f) );

                    if (refresh_rate != 0)
                    {
                        ImGui::Text("%d Hz",
                                    refresh_rate);
                    }
                    else
                    {
                        ImGui::Text("Unknown");
                    }

                    ImGui::PopStyleColor();
                }
            }

            ImGui::SetCursorPosY(WINDOW_HEIGHT / 8 * 7);

            {
                const auto is_disabled = (m_specified_glquake_file_path.size() == 0);

                if (is_disabled)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button("Launch",
                                  ImVec2(ImGui::GetContentRegionAvail().x, 0) ) )
                {
                    m_worker_thread_must_die = true;
                }

                if (is_disabled)
                {
                    ImGui::EndDisabled();
                }
            }
        }
    }
    ImGui::End();
}

bool Launcher::UI::show_glquake_exe_path_dialog(std::wstring* out_file_path_ptr,
                                                std::wstring* out_file_path_with_name_ptr)
{
    OPENFILENAMEW open_file_name_descriptor           = {};
    bool          result                              = false;
    wchar_t       result_file_name         [MAX_PATH] = {};

    open_file_name_descriptor.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    open_file_name_descriptor.hwndOwner   = HWND_DESKTOP;
    open_file_name_descriptor.lStructSize = sizeof(open_file_name_descriptor);
    open_file_name_descriptor.lpstrFilter  = L"GLQuake Executable (glquake.exe)\0glquake.exe\0";
    open_file_name_descriptor.lpstrFile    = result_file_name;
    open_file_name_descriptor.nMaxFile     = sizeof(result_file_name);

    if (::GetOpenFileNameW(&open_file_name_descriptor) == 0)
    {
        ::MessageBox(HWND_DESKTOP,
                     "You must provide a path to glquake.exe in order to run the tool.",
                     "Error",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    *out_file_path_ptr           = std::wstring(result_file_name,
                                                0,
                                                open_file_name_descriptor.nFileOffset);
    *out_file_path_with_name_ptr = std::wstring(result_file_name);

    result = true;
end:
    return result;
}

void Launcher::UI::update_vr_device_name_vec()
{
    const auto                      active_vr_backend                = Common::Misc::get_vr_backend_for_text_string(m_available_vr_runtime_name_ptr_vec.at(m_n_selected_vr_runtime) );
    const std::vector<std::string>* available_vr_device_name_vec_ptr = nullptr;

    m_available_vr_device_name_ptr_vec.clear();

    if (active_vr_backend != Common::VRBackend::UNKNOWN)
    {
        m_vr_support_ptr->enumerate_devices(active_vr_backend,
                                           &available_vr_device_name_vec_ptr);

        for (const auto& current_vr_device_name : *available_vr_device_name_vec_ptr)
        {
            m_available_vr_device_name_ptr_vec.emplace_back(current_vr_device_name.c_str() );
        }
    }
}

bool Launcher::UI::wait_until_closed()
{
    m_worker_thread.join();

    return (m_state_ptr->get_glquake_exe_file_path_ptr() != nullptr);
}