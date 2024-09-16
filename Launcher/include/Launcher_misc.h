/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(LAUNCHER_MISC_H)
#define LAUNCHER_MISC_H

#include <string>

namespace Launcher
{
    class State;

    enum class VRBackend : uint8_t
    {
        LIBOVR,
        OPENXR,

        UNKNOWN
    };

    namespace Misc
    {
        constexpr const wchar_t* GLQUAKE_EXE_FILE_NAME = L"glquake.exe";

        bool check_glquake_exe_compatibility(State*         in_state_ptr);
        bool check_glquake_exe_compatibility(const wchar_t* in_file_path_ptr,
                                             const wchar_t* in_file_name_ptr,
                                             std::string*   out_error_string_ptr);

        const uint8_t* get_u8_text_string_for_vr_backend(const VRBackend&   in_vr_backend);
        VRBackend      get_vr_backend_for_text_string   (const std::string& in_string);

    }
}

#endif /* LAUNCHER_MISC_H */