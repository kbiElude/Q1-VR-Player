/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(COMMON_MISC_H)
#define COMMON_MISC_H

#include <string>

namespace Common
{
    enum class VRBackend : uint8_t
    {
        LIBOVR,
        OPENXR,

        UNKNOWN
    };

    namespace Misc
    {
        const uint8_t* get_u8_text_string_for_vr_backend(const VRBackend&   in_vr_backend);
        VRBackend      get_vr_backend_for_text_string   (const std::string& in_string);
    }
}

#endif /* COMMON_MISC_H */