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
#include "Launcher_vr_support.h"

int main()
{
    std::wstring glquake_exe_file_name   = L"glquake.exe";
    int          result                  = EXIT_FAILURE;
    auto         state_ptr               = Launcher::State::create();
    std::string  vr_player_dll_file_name =  "VRPlayer.dll";


    /* Obtain GLQuake location from previous runs if available */
    {
        const auto glquake_exe_file_path_ptr = state_ptr->get_glquake_exe_file_path_ptr();

        if (glquake_exe_file_path_ptr != nullptr)
        {
            glquake_exe_file_name = *glquake_exe_file_path_ptr + glquake_exe_file_name;
        }
    }

    /* Is it there? */
    if (::GetFileAttributesW(glquake_exe_file_name.c_str() ) == INVALID_FILE_ATTRIBUTES)
    {
        /* Nope. Ask the user to point to the file. */
        OPENFILENAMEW open_file_name_descriptor           = {};
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

        glquake_exe_file_name = std::wstring(result_file_name);

        state_ptr->set_glquake_exe_file_path(
            std::wstring(result_file_name,
                         0,
                         open_file_name_descriptor.nFileOffset)
        );
    }

    /* Add a check to ensure we're running a supported GLQuake executable. */
    {
        WIN32_FILE_ATTRIBUTE_DATA file_attrib_data = {};

        if (::GetFileAttributesExW(glquake_exe_file_name.c_str(),
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

    /* Append args required for the tool to work as expected with glquake. */
    {
        auto& eye_texture_extents = *state_ptr->get_eye_texture_extents_ptr();

        glquake_exe_file_name += std::wstring(L" -window -fullsbar -width ") +
                                 std::to_wstring(eye_texture_extents.at(0) ) +
                                 std::wstring(L" -height ")                  +
                                 std::to_wstring(eye_texture_extents.at(1) );
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
        PROCESS_INFORMATION process_info;
        STARTUPINFOW        startup_info;

        memset(&process_info,
               0,
               sizeof(process_info) );
        memset(&startup_info,
               0,
               sizeof(startup_info) );

        startup_info.cb = sizeof(startup_info);

        if (::DetourCreateProcessWithDllW(nullptr,
                                          const_cast<LPWSTR>(glquake_exe_file_name.c_str() ),
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