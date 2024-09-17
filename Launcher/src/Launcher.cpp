/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include <Windows.h>
#include <array>
#include <assert.h>
#include <string>
#include "common_defines.inl"
#include "deps/Detours/src/detours.h"
#include "Launcher_misc.h"
#include "Launcher_state.h"
#include "Launcher_ui.h"
#include "Launcher_vr_support.h"


int main()
{
    bool         should_show_ui          = false;
    int          result                  = EXIT_FAILURE;
    auto         state_ptr               = Launcher::State::create    ();
    std::string  vr_player_dll_file_name = "VRPlayer.dll";
    auto         vr_support_ptr          = Launcher::VRSupport::create();

    /* Is there at least one VR backend available to work with? */
    if (vr_support_ptr == nullptr)
    {
        ::MessageBox(HWND_DESKTOP,
                     "Could not initialize VR support\n\nPlease make sure your VR system is enabled & fully functional before retrying.",
                     "Error",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    /* Should we show a configuration window before we launch the game? */
    should_show_ui = (state_ptr->get_active_vr_backend               ()                 == Launcher::VRBackend::UNKNOWN) ||
                     (state_ptr->get_glquake_exe_file_path_ptr       ()                 == nullptr)                      ||
                     (Launcher::Misc::check_glquake_exe_compatibility(state_ptr.get() ) == false);

    if (should_show_ui)
    {
        auto ui_ptr = Launcher::UI::create(state_ptr.get     (),
                                           vr_support_ptr.get() );

        assert(vr_support_ptr != nullptr);

        if (!ui_ptr->wait_until_closed() )
        {
            goto end;
        }
    }

    if (state_ptr->get_active_vr_backend() == Launcher::VRBackend::UNKNOWN)
    {
        ::MessageBox(HWND_DESKTOP,
                     "No VR runtime selected.\n",
                     "Error",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    if (state_ptr->get_glquake_exe_file_path_ptr() == nullptr)
    {
        ::MessageBox(HWND_DESKTOP,
                     "GLQuake.exe could not be found at the specified location.",
                     "Error",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    /* Now look for VRPlayer.dll .. */
    {
        char working_dir[MAX_PATH] = {};

        if (::GetCurrentDirectory(sizeof(working_dir),
                                  working_dir) == 0)
        {
            ::MessageBox(HWND_DESKTOP,
                         "Failed to identify working directory.",
                         "Error",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        vr_player_dll_file_name = std::string(working_dir) + "\\" + vr_player_dll_file_name;

        if (::GetFileAttributes(vr_player_dll_file_name.c_str() ) == INVALID_FILE_ATTRIBUTES)
        {
            ::MessageBox(HWND_DESKTOP,
                         "VRPlayer.dll not found in the working directory.",
                         "Error",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    // Launch GLQuake with APIInterceptor attached.
    {
        std::wstring        glquake_exe_with_args;
        PROCESS_INFORMATION process_info;
        STARTUPINFOW        startup_info;

        memset(&process_info,
               0,
               sizeof(process_info) );
        memset(&startup_info,
               0,
               sizeof(startup_info) );

        startup_info.cb = sizeof(startup_info);

        /* Append args required for the tool to work as expected with glquake. */
        {
            std::array<uint32_t, 2> eye_texture_extents;

            vr_support_ptr->get_eye_texture_extents(state_ptr->get_active_vr_backend(),
                                                   &eye_texture_extents);

            if (eye_texture_extents.at(0) == 0  ||
                eye_texture_extents.at(1) == 0)
            {
                ::MessageBox(HWND_DESKTOP,
                             "Failed to determine eye texture resolution.",
                             "Error",
                             MB_OK | MB_ICONERROR);

                goto end;
            }
    
            glquake_exe_with_args = *state_ptr->get_glquake_exe_file_path_ptr()          +
                                     std::wstring(Launcher::Misc::GLQUAKE_EXE_FILE_NAME) +
                                     std::wstring(L" -window -fullsbar -width ")         +
                                     std::to_wstring(eye_texture_extents.at(0) )         +
                                     std::wstring(L" -height ")                          +
                                     std::to_wstring(eye_texture_extents.at(1) );
        }

        if (::DetourCreateProcessWithDllW(nullptr,
                                          const_cast<LPWSTR>(glquake_exe_with_args.c_str() ),
                                          nullptr,                                             /* lpProcessAttributes */
                                          nullptr,                                             /* lpThreadAttributes  */
                                          FALSE,                                               /* bInheritHandles     */
                                          CREATE_DEFAULT_ERROR_MODE,
                                          nullptr,                                             /* lpEnvironment       */
                                          state_ptr->get_glquake_exe_file_path_ptr()->c_str(), /* lpCurrentDirectory  */
                                         &startup_info,
                                         &process_info,
                                          vr_player_dll_file_name.c_str(),
                                          nullptr) != TRUE)                                    /* pfCreateProcessA    */
        {
            std::string error = "Could not launch GLQuake with APIInterceptor attached.";

            ::MessageBox(HWND_DESKTOP,
                         error.c_str(),
                         "Error",
                         MB_OK | MB_ICONERROR);
        }

        #if defined(_DEBUG)
        {
            ::WaitForSingleObject(process_info.hProcess,
                                  INFINITE);
        }
        #endif
    }

    result = EXIT_SUCCESS;
end:
    return result;
}