/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_misc.h"
#include "Launcher_state.h"
#include "Launcher_vr_support.h"
#include "OVR_CAPI_GL.h"
#include <Windows.h>
#include <assert.h>

Launcher::VRSupport::VRSupport()
{
    /* Stub */
}

Launcher::VRSupport::~VRSupport()
{
    /* Stub */
}

Launcher::VRSupportUniquePtr Launcher::VRSupport::create()
{
    Launcher::VRSupportUniquePtr result_ptr;

    result_ptr.reset(
        new VRSupport()
    );

    assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void Launcher::VRSupport::enumerate_devices(const VRBackend&                 in_backend,
                                            const std::vector<std::string>** out_device_name_vec_ptr_ptr)
{
    switch (in_backend)
    {
        case VRBackend::LIBOVR: *out_device_name_vec_ptr_ptr = &m_libovr_backend_info.device_name_vec; break;
        // case VRBackend::OPENXR:

        default:
        {
            assert(false);
        }
    }
}

void Launcher::VRSupport::get_eye_texture_extents(const VRBackend&         in_backend,
                                                  std::array<uint32_t, 2>* out_u32vec2_ptr)
{
    switch (in_backend)
    {
        case VRBackend::LIBOVR: *out_u32vec2_ptr = m_libovr_backend_info.eye_texture_extents; break;
        // case VRBackend::OPENXR:

        default:
        {
            assert(false);
        }
    }
}

void Launcher::VRSupport::get_refresh_rate(const VRBackend& in_backend,
                                           uint32_t*        out_refresh_rate_ptr)
{
    switch (in_backend)
    {
        case VRBackend::LIBOVR: *out_refresh_rate_ptr = m_libovr_backend_info.refresh_rate; break;
        // case VRBackend::OPENXR:

        default:
        {
            assert(false);
        }
    }
}

bool Launcher::VRSupport::init()
{
    bool result = false;

    init_libovr();

    return true;
}

void Launcher::VRSupport::init_libovr()
{
    ovrGraphicsLuid ovr_gfx_luid    = {};
    ovrSession      ovr_session_ptr = nullptr;
    bool            result          = false;

    /* 1. Try to create an ovr session instance. */
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

    /* 2. Cache properties we're going to need to expose. */
    {
        const auto hmd_desc         = ::ovr_GetHmdDesc       (ovr_session_ptr);
        const auto fov_texture_size = ::ovr_GetFovTextureSize(ovr_session_ptr,
                                                              ::ovrEye_Left,
                                                              hmd_desc.DefaultEyeFov[0],
                                                              1.0f); /* pixelsPerDisplayPixels */

        m_libovr_backend_info.device_name_vec.emplace_back(hmd_desc.ProductName);

        m_libovr_backend_info.eye_texture_extents.at(0) = fov_texture_size.w;
        m_libovr_backend_info.eye_texture_extents.at(1) = fov_texture_size.h;
        m_libovr_backend_info.refresh_rate              = static_cast<uint32_t>(hmd_desc.DisplayRefreshRate);
    }

    /* 3. All done */
    m_libovr_backend_info.is_supported = true;
end:
    ;
}

bool Launcher::VRSupport::is_vr_backend_supported(const VRBackend& in_backend)
{
    bool result = false;

    switch (in_backend)
    {
        case VRBackend::LIBOVR: result = m_libovr_backend_info.is_supported; break;
        case VRBackend::OPENXR: result = m_openxr_backend_info.is_supported; break;

        default:
        {
            assert(false);
        }
    }

    return result;
}