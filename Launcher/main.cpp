/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include <Windows.h>
#include <array>
#include <string>
#include "common_defines.inl"
#include "common_file_serializer.h"
#include "deps/Detours/src/detours.h"
#include "OVR_CAPI_GL.h"

static const std::map<std::string, Variant::Type> g_serializer_settings =
{
    {"GLQuakeExePath", Variant::Type::U8_TEXT_STRING}
};

bool get_native_resolution(std::array<uint32_t, 2>* out_u32vec2_ptr)
{
    ovrGraphicsLuid ovr_gfx_luid    = {};
    ovrSession      ovr_session_ptr = nullptr;
    bool            result          = false;

    /* 1. Try to create ovr session instance. */
    {
        ovrInitParams initParams =
        {
            ovrInit_RequestVersion | ovrInit_FocusAware,
            OVR_MINOR_VERSION,
            NULL,
            0,
            0
        };

        if (!OVR_SUCCESS(::ovr_Initialize(&initParams) ))
        {
            goto end;
        }
    }

    if (!OVR_SUCCESS(::ovr_Create(&ovr_session_ptr,
                                  &ovr_gfx_luid) ))
    {
        goto end;
    }

    {
        const auto hmd_desc         = ::ovr_GetHmdDesc       (ovr_session_ptr);
        const auto fov_texture_size = ::ovr_GetFovTextureSize(ovr_session_ptr,
                                                              ::ovrEye_Left,
                                                              hmd_desc.DefaultEyeFov[0],
                                                              1.0f); /* pixelsPerDisplayPixels */

        out_u32vec2_ptr->at(0) = fov_texture_size.w;
        out_u32vec2_ptr->at(1) = fov_texture_size.h;
    }

    result = true;
end:
    if (ovr_session_ptr)
    {
        ::ovr_Destroy(ovr_session_ptr);
    }

    if (!result)
    {
        ::MessageBox(HWND_DESKTOP,
                     "Failed to initialize LibOVR. This usually indicates:\n"
                     "\n"
                     "1. You do not have Oculus Quest.\n"
                     "2. Your Quest is not in Oculus Link mode.\n"
                     "3. Your Quest has hibernated. Put the glasses on and make sure they're on.\n"
                     "\n",
                     "Error",
                     MB_OK | MB_ICONERROR);
    }

    return result;
}


int main()
{
    std::wstring glquake_exe_file_name   = L"glquake.exe";
    std::wstring glquake_exe_file_path;
    int          result                  = EXIT_FAILURE;
    std::string  vr_player_dll_file_name =  "VRPlayer.dll";

    /* Determine VR eye texture resolution */
    std::array<uint32_t, 2> vr_eye_resolution{};

    if (!get_native_resolution(&vr_eye_resolution) )
    {
        goto end;
    }

    /* Try to serialize application's location from settings file first.. */
    {
        auto serializer_ptr = FileSerializer::create_for_reading("q1_vr_launcher_settings.txt",
                                                                 g_serializer_settings);

        if (serializer_ptr != nullptr)
        {
            uint32_t       file_path_u8_n_bytes = 0;
            const uint8_t* file_path_u8_ptr     = nullptr;

            if (serializer_ptr->get_u8_text_string(g_serializer_settings.begin()->first.c_str(),
                                                  &file_path_u8_ptr,
                                                  &file_path_u8_n_bytes) )
            {
                /* TODO: This is not going to work at all for paths with wide characters. */
                auto string_u8 = std::string(reinterpret_cast<const char*>(file_path_u8_ptr),
                                             file_path_u8_n_bytes);

                glquake_exe_file_path = std::wstring(string_u8.begin(),
                                                     string_u8.end  () );

                glquake_exe_file_name = glquake_exe_file_path + glquake_exe_file_name;
            }
        }
    }

    /* Identify where glquake.exe is located. */
    if (::GetFileAttributesW(glquake_exe_file_name.c_str() ) == INVALID_FILE_ATTRIBUTES)
    {
        /* Not in the working directory. Ask the user to point to the file. */
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
        glquake_exe_file_path = std::wstring(result_file_name,
                                             0,
                                             open_file_name_descriptor.nFileOffset);
    }

    /* Store the specified location for future reuse. */
    {
        auto serializer_ptr = FileSerializer::create_for_writing("q1_vr_launcher_settings.txt",
                                                                 g_serializer_settings);

        if (serializer_ptr != nullptr)
        {
            std::string path_u8 = std::string(glquake_exe_file_path.begin(),
                                              glquake_exe_file_path.end  () );

            serializer_ptr->set_u8_text_string(g_serializer_settings.begin()->first.c_str(),
                                               reinterpret_cast<const uint8_t*>          (path_u8.c_str() ),
                                               path_u8.size                              () );
        }
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
                              "Your GLQuake executable uses an unrecognized version. Only 0.98 Alpha are confirmed to work correctly (http://www.quaketerminus.com/nqexes/glquake098alpha.zip).",
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

    /* Append args required for the tool to work as expected with glquake. */
    glquake_exe_file_name += std::wstring(L" -window -fullsbar -width ") +
                             std::to_wstring(vr_eye_resolution.at(0) )   +
                             std::wstring(L" -height ")                  +
                             std::to_wstring(vr_eye_resolution.at(1) );

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
                                          nullptr,                          /* lpProcessAttributes */
                                          nullptr,                          /* lpThreadAttributes  */
                                          FALSE,                            /* bInheritHandles     */
                                          CREATE_DEFAULT_ERROR_MODE,
                                          nullptr,                          /* lpEnvironment       */
                                          glquake_exe_file_path.c_str(),    /* lpCurrentDirectory  */
                                         &startup_info,
                                         &process_info,
                                          vr_player_dll_file_name.c_str(),
                                          nullptr) != TRUE)                 /* pfCreateProcessA    */
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