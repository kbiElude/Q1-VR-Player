/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(LAUNCHER_MISC_H)
#define LAUNCHER_MISC_H

#include <string>
#include <vector>

namespace Launcher
{
    class State;

    namespace Misc
    {
        constexpr const wchar_t* GLQUAKE_EXE_FILE_NAME = L"glquake.exe";

        std::vector<std::string> get_current_process_env_var_vec();
        std::vector<uint8_t>     get_u8_vec_for_env_var_vec     (const std::vector<std::string>& in_env_var_vec);

        bool check_glquake_exe_compatibility(State*         in_state_ptr);
        bool check_glquake_exe_compatibility(const wchar_t* in_file_path_ptr,
                                             const wchar_t* in_file_name_ptr,
                                             std::string*   out_error_string_ptr);
    }
}

#endif /* LAUNCHER_MISC_H */