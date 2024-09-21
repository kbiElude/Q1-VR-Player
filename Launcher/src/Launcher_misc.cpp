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

    glquake_exe_file_path = (in_file_path_ptr != nullptr) ? std::wstring(in_file_path_ptr) + in_file_name_ptr
                                                          :                                  in_file_name_ptr;

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

std::vector<std::string> Launcher::Misc::get_current_process_env_var_vec()
{
    const char*              env_string_block_ptr = ::GetEnvironmentStrings();
    std::vector<std::string> result_vec;
    const char*              traveler_ptr         = env_string_block_ptr;

    while (*traveler_ptr != 0)
    {
        if (*(traveler_ptr + 1) == 0)
        {
            break;
        }
        else
        {
            auto last_env_var_char_ptr = strchr(traveler_ptr,
                                                '\0');
            auto new_env_var           = std::string(traveler_ptr,
                                                     last_env_var_char_ptr);

            result_vec.emplace_back(new_env_var);

            traveler_ptr = last_env_var_char_ptr + 1;
        }
    }

    ::FreeEnvironmentStrings(const_cast<char*>(env_string_block_ptr) );

    return result_vec;
}

std::vector<uint8_t> Launcher::Misc::get_u8_vec_for_env_var_vec(const std::vector<std::string>& in_env_var_vec)
{
    std::vector<uint8_t> result_u8_vec;

    for (const auto& current_env_var : in_env_var_vec)
    {
        const auto n_start_character = static_cast<uint32_t>(result_u8_vec.size() );

        result_u8_vec.resize(result_u8_vec.size() + current_env_var.size() + 1 /* terminator */);

        memcpy(result_u8_vec.data   () + n_start_character,
               current_env_var.c_str(),
               current_env_var.size () );
    }

    result_u8_vec.resize(result_u8_vec.size() + 1 /* extra terminator */);

    return result_u8_vec;
}