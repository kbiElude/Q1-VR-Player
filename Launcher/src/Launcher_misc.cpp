/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
 #include "Launcher_misc.h"
 #include "Launcher_state.h"
 #include <Windows.h>
 #include <assert.h>
 #include <string>

bool Launcher::Misc::check_glquake_exe_compatibility(State* in_state_ptr)
{
    std::string    dummy;
    const auto     glquake_exe_file_path_ptr     = in_state_ptr->get_glquake_exe_file_path_ptr();
    const wchar_t* glquake_exe_file_path_raw_ptr = (glquake_exe_file_path_ptr != nullptr) ? glquake_exe_file_path_ptr->c_str()
                                                                                          : nullptr;

    return check_glquake_exe_compatibility(glquake_exe_file_path_raw_ptr,
                                           GLQUAKE_EXE_FILE_NAME,
                                          &dummy);
}

bool Launcher::Misc::check_glquake_exe_compatibility(const wchar_t* in_file_path_ptr,
                                                     const wchar_t* in_file_name_ptr,
                                                     std::string*   out_error_string_ptr)
{
    WIN32_FILE_ATTRIBUTE_DATA file_attrib_data = {};
    bool                      result           = false;

    std::wstring glquake_exe_file_path;

    glquake_exe_file_path = (in_file_path_ptr != nullptr) ? *in_file_path_ptr + in_file_name_ptr
                                                          :                     in_file_name_ptr;

    if (::GetFileAttributesExW(glquake_exe_file_path.c_str(),
                               GetFileExInfoStandard,
                              &file_attrib_data) != 0)
    {
        if (file_attrib_data.nFileSizeLow != 504832) //glquake 0.98 alpha
        {
            *out_error_string_ptr = "Your GLQuake executable uses an unrecognized version. Only 0.98 Alpha is confirmed to work correctly (http://www.quaketerminus.com/nqexes/glquake098alpha.zip).";

            goto end;
        }
    }
    else
    {
        *out_error_string_ptr = "Could not query file attributes of your GLQuake executable. Compatibility check could not be executed.";

        goto end;
    }

    result = true;
end:
    return result;
}

const uint8_t* Launcher::Misc::get_u8_text_string_for_vr_backend(const VRBackend& in_vr_backend)
{
    const char* result_ptr = "!?";

    switch (in_vr_backend)
    {
        case VRBackend::LIBOVR: result_ptr = "LibOVR"; break;
        case VRBackend::OPENXR: result_ptr = "OpenXR"; break;

        default:
        {
            assert(false);
        }
    }

    return reinterpret_cast<const uint8_t*>(result_ptr);
}

Launcher::VRBackend Launcher::Misc::get_vr_backend_for_text_string(const std::string& in_string)
{
    auto result = Launcher::VRBackend::UNKNOWN;

    if (in_string == "LibOVR")
    {
        result = Launcher::VRBackend::LIBOVR;
    }
    else
    if (in_string == "OpenXR")
    {
        result = Launcher::VRBackend::OPENXR;
    }
    else
    {
        assert(false);
    }

    return result;
}