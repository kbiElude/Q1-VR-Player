/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_misc.h"
#include <assert.h>

const uint8_t* Common::Misc::get_u8_text_string_for_vr_backend(const VRBackend& in_vr_backend)
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

Common::VRBackend Common::Misc::get_vr_backend_for_text_string(const std::string& in_string)
{
    auto result = Common::VRBackend::UNKNOWN;

    if (in_string == "LibOVR")
    {
        result = Common::VRBackend::LIBOVR;
    }
    else
    if (in_string == "OpenXR")
    {
        result = Common::VRBackend::OPENXR;
    }
    else
    {
        assert(false);
    }

    return result;
}