/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_UI_H
#define LAUNCHER_UI_H

#include <array>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct GLFWwindow;
struct ImGuiContext;

namespace Launcher
{
    class State;
    class UI;
    class VRSupport;

    typedef std::unique_ptr<UI> UIUniquePtr;


    class UI
    {
    public:
        /* Public funcs */
        static UIUniquePtr create(State*     in_state_ptr,
                                  VRSupport* in_vr_support_ptr);

        bool wait_until_closed();

        ~UI();

    private:
        /* Private funcs */
        UI(State*     in_state_ptr,
           VRSupport* in_vr_support_ptr);

        void execute                     ();
        bool init                        ();
        void render                      ();
        bool show_glquake_exe_path_dialog(std::wstring* out_file_path_ptr,
                                          std::wstring* out_file_path_with_name_ptr);

        void handle_set_glquake_location_button_click_event();
        void update_vr_device_name_vec                     ();

        /* Private vars */
        GLFWwindow*   m_glfw_window_ptr;
        ImGuiContext* m_imgui_context_ptr;

        State*      m_state_ptr;
        VRSupport*  m_vr_support_ptr;
        std::thread m_worker_thread;
        bool        m_worker_thread_must_die;

        std::vector<const char*> m_available_vr_device_name_ptr_vec;
        int                      m_n_selected_vr_device;

        std::vector<const char*> m_available_vr_runtime_name_ptr_vec;
        int                      m_n_selected_vr_runtime;

        std::wstring m_specified_glquake_file_path;
    };
}

#endif /* LAUNCHER_VR_SUPPORT_H */