/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include <Windows.h>
#include <array>
#include <string>
#include "common_defines.inl"
#include "deps/Detours/src/detours.h"
#include "Launcher_state.h"
#include "Launcher_ui.h"
#include "Launcher_vr_support.h"

static const wchar_t* g_glquake_exe_file_name_ptr = L"glquake.exe";


bool check_glquake_exe_compatibility(Launcher::State* in_state_ptr)
{
    WIN32_FILE_ATTRIBUTE_DATA file_attrib_data = {};
    bool                      result           = false;

    const auto   glquake_exe_file_path_ptr = in_state_ptr->get_glquake_exe_file_path_ptr();
    std::wstring glquake_exe_file_path;

    glquake_exe_file_path = (glquake_exe_file_path_ptr != nullptr) ? *glquake_exe_file_path_ptr + g_glquake_exe_file_name_ptr
                                                                   :                              g_glquake_exe_file_name_ptr;

    if (::GetFileAttributesExW(glquake_exe_file_path.c_str(),
                               GetFileExInfoStandard,
                              &file_attrib_data) != 0)
    {
        if (file_attrib_data.nFileSizeLow != 504832) //glquake 0.98 alpha
        {
            ::MessageBoxA(HWND_DESKTOP,
                          "Your GLQuake executable uses an unrecognized version. Only 0.98 Alpha is confirmed to work correctly (http://www.quaketerminus.com/nqexes/glquake098alpha.zip).",
                          "Warning",
                          MB_OK | MB_ICONWARNING);

            goto end;
        }
    }
    else
    {
        ::MessageBoxA(HWND_DESKTOP,
                      "Could not query file attributes of your GLQuake executable. Compatibility check could not be executed.",
                      "Warning",
                      MB_OK | MB_ICONWARNING);
    }

    result = true;
end:
    return result;
}

int main()
{
    bool         should_show_ui          = false;
    int          result                  = EXIT_FAILURE;
    auto         state_ptr               = Launcher::State::create();
    std::string  vr_player_dll_file_name =  "VRPlayer.dll";

    /* Should we show a configuration window before we launch the game? */
    should_show_ui = (state_ptr->get_glquake_exe_file_path_ptr()                == nullptr) ||
                     (check_glquake_exe_compatibility         (state_ptr.get() ) == false);

    if (should_show_ui)
    {
        auto ui_ptr = Launcher::UI::create(state_ptr.get() );
    }

    /* Determine eye texture resolution */
    if (state_ptr->get_eye_texture_extents_ptr() == nullptr)
    {
        std::array<int32_t, 2> eye_texture_extents;

        if (!Launcher::VRSupport::get_eye_texture_extents(&eye_texture_extents) )
        {
            goto end;
        }

        state_ptr->set_eye_texture_extents(eye_texture_extents);
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
            auto& eye_texture_extents = *state_ptr->get_eye_texture_extents_ptr();
    
            glquake_exe_with_args = *state_ptr->get_glquake_exe_file_path_ptr()  +
                                     std::wstring(g_glquake_exe_file_name_ptr)   +
                                     std::wstring(L" -window -fullsbar -width ") +
                                     std::to_wstring(eye_texture_extents.at(0) ) +
                                     std::wstring(L" -height ")                  +
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