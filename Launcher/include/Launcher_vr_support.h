/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_VR_SUPPORT_H
#define LAUNCHER_VR_SUPPORT_H

#include <array>

namespace Launcher
{
    class VRSupport
    {
    public:
        /* Public funcs */
        static bool get_eye_texture_extents(std::array<int32_t, 2>* out_u32vec2_ptr);
    };
}

#endif /* LAUNCHER_VR_SUPPORT_H */