/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_vr_support.h"
#include "OVR_CAPI_GL.h"
#include <Windows.h>

bool Launcher::VRSupport::get_eye_texture_extents(std::array<int32_t, 2>* out_u32vec2_ptr)
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