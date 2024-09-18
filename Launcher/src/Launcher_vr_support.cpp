/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_dummy_gl_context.h"
#include "Launcher_misc.h"
#include "Launcher_state.h"
#include "Launcher_vr_support.h"
#include "OVR_CAPI_GL.h"
#include <Windows.h>
#include <assert.h>

/* OpenXR headers need a few extra nudges to include the right stuff.. */
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"


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

void Launcher::VRSupport::enumerate_devices(const Common::VRBackend&         in_backend,
                                            const std::vector<std::string>** out_device_name_vec_ptr_ptr)
{
    switch (in_backend)
    {
        case Common::VRBackend::LIBOVR: *out_device_name_vec_ptr_ptr = &m_libovr_backend_info.device_name_vec; break;
        case Common::VRBackend::OPENXR: *out_device_name_vec_ptr_ptr = &m_openxr_backend_info.device_name_vec; break;

        default:
        {
            assert(false);
        }
    }
}

void Launcher::VRSupport::get_eye_texture_extents(const Common::VRBackend& in_backend,
                                                  std::array<uint32_t, 2>* out_u32vec2_ptr)
{
    switch (in_backend)
    {
        case Common::VRBackend::LIBOVR: *out_u32vec2_ptr = m_libovr_backend_info.eye_texture_extents; break;
        case Common::VRBackend::OPENXR: *out_u32vec2_ptr = m_openxr_backend_info.eye_texture_extents; break;

        default:
        {
            assert(false);
        }
    }
}

void Launcher::VRSupport::get_refresh_rate(const Common::VRBackend& in_backend,
                                           uint32_t*                out_refresh_rate_ptr)
{
    switch (in_backend)
    {
        case Common::VRBackend::LIBOVR: *out_refresh_rate_ptr = m_libovr_backend_info.refresh_rate; break;
        case Common::VRBackend::OPENXR: *out_refresh_rate_ptr = m_openxr_backend_info.refresh_rate; break;

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
    init_openxr();

    result = (m_libovr_backend_info.is_supported) ||
             (m_openxr_backend_info.is_supported);

    return result;
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

    ::ovr_Destroy(ovr_session_ptr);
end:
    ;
}

void Launcher::VRSupport::init_openxr()
{
    XrInstance xr_instance  = {};
    XrSession  xr_session   = {};
    XrSystemId xr_system_id = {};

    /* In order to create an XR session, we're going to need a GL context bound to current thread. */
    DummyGLContextUniquePtr gl_context_ptr = DummyGLContext::create();

    if (gl_context_ptr == nullptr)
    {
        assert(gl_context_ptr != nullptr);

        goto end;
    }

    {
        static const char* xr_fb_display_refresh_rate_string = "XR_FB_display_refresh_rate";
        static const char* xr_khr_opengl_extension_string    = "XR_KHR_opengl_enable";

        bool                               is_xr_fb_display_refresh_rate_available = false;
        std::vector<XrExtensionProperties> xr_ext_props_vec;

        auto is_ext_available_func = [&](const char* in_extension_name_ptr) -> bool
        {
            return std::find_if(xr_ext_props_vec.begin(),
                                xr_ext_props_vec.end  (),
                                [&](const XrExtensionProperties& in_props)
                                {
                                    return strcmp(in_props.extensionName,
                                                  in_extension_name_ptr) == 0;
                                }) != xr_ext_props_vec.end();
        };

        /* 1. Set up OpenXR instance first.. */
        {
            uint32_t n_extensions_available = 0;

            if (!XR_SUCCEEDED(::xrEnumerateInstanceExtensionProperties(nullptr,                 /* layerName             */
                                                                       0,                       /* propertyCapacityInput */
                                                                       &n_extensions_available,
                                                                       nullptr) ))              /* properties            */
            {
                goto end;
            }

            xr_ext_props_vec.resize(n_extensions_available,
                                    XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES, nullptr});

            if (!XR_SUCCEEDED(::xrEnumerateInstanceExtensionProperties(nullptr,                    /* layerName             */
                                                                       n_extensions_available,     /* propertyCapacityInput */
                                                                      &n_extensions_available,
                                                                       xr_ext_props_vec.data()) )) /* properties            */
            {
                goto end;
            }
        }

        if (!is_ext_available_func(xr_khr_opengl_extension_string) )
        {
            goto end;
        }

        is_xr_fb_display_refresh_rate_available = is_ext_available_func(xr_fb_display_refresh_rate_string);

        {
            XrInstanceCreateInfo     create_info                = {};
            std::vector<const char*> enabled_extension_name_vec = {xr_khr_opengl_extension_string};

            if (is_xr_fb_display_refresh_rate_available)
            {
                enabled_extension_name_vec.emplace_back(xr_fb_display_refresh_rate_string);
            }

            create_info.applicationInfo.apiVersion         = XR_API_VERSION_1_0;
            create_info.applicationInfo.applicationVersion = 1;
            create_info.applicationInfo.engineVersion      = 0;
            create_info.createFlags                        = 0;
            create_info.enabledApiLayerCount               = 0;
            create_info.enabledApiLayerNames               = nullptr;
            create_info.enabledExtensionCount              = static_cast<uint32_t>          (enabled_extension_name_vec.size() );
            create_info.enabledExtensionNames              = enabled_extension_name_vec.data();
            create_info.type                               = XR_TYPE_INSTANCE_CREATE_INFO;

            strcpy(create_info.applicationInfo.applicationName,
                   "Q1 VR Player");
            strcpy(create_info.applicationInfo.engineName,
                   "Q1 VR Player");

            if (!XR_SUCCEEDED(::xrCreateInstance(&create_info,
                                                 &xr_instance) ))
            {
                goto end;
            }
        }

        /* 2. Is there a head-mounted display system available? */
        {
            XrSystemGetInfo get_info = {};

            get_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
            get_info.next       = nullptr;
            get_info.type       = XR_TYPE_SYSTEM_GET_INFO;

            if (!XR_SUCCEEDED(::xrGetSystem(xr_instance,
                                           &get_info,
                                           &xr_system_id) ))
            {
                goto end;
            }
        }

        /* 3. OpenXR requires us to call xrGetOpenGLGraphicsRequirementsKHR() before we open an XR session. */
        {
            PFN_xrGetOpenGLGraphicsRequirementsKHR pfn_xr_get_opengl_gfx_requirements_khr = nullptr;
            XrGraphicsRequirementsOpenGLKHR        xr_gfx_requirements                    = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};

            if (!XR_SUCCEEDED(xrGetInstanceProcAddr(xr_instance,
                                                    "xrGetOpenGLGraphicsRequirementsKHR",
                                                    reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xr_get_opengl_gfx_requirements_khr) )))
            {
                goto end;
            }

            if (!XR_SUCCEEDED(pfn_xr_get_opengl_gfx_requirements_khr(xr_instance,
                                                                     xr_system_id,
                                                                    &xr_gfx_requirements) ))
            {
                goto end;
            }
        }

        /* 4. All right, create that session. */
        {
            XrSessionCreateInfo             create_info{};
            XrGraphicsBindingOpenGLWin32KHR gfx_binding_gl_win32 = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};

            gfx_binding_gl_win32.hDC   = gl_context_ptr->get_dc  ();
            gfx_binding_gl_win32.hGLRC = gl_context_ptr->get_glrc();

            create_info.createFlags = 0;
            create_info.next        = &gfx_binding_gl_win32;
            create_info.systemId    = xr_system_id;
            create_info.type        = XR_TYPE_SESSION_CREATE_INFO;

            if (!XR_SUCCEEDED(::xrCreateSession(xr_instance,
                                               &create_info,
                                               &xr_session)) )
            {
                goto end;
            }
        }

        /* 5. Retrieve basic system properties. */
        {
            XrSystemProperties xr_system_props = {XR_TYPE_SYSTEM_PROPERTIES};

            if (!XR_SUCCEEDED(::xrGetSystemProperties(xr_instance,
                                                      xr_system_id,
                                                     &xr_system_props)))
            {
                goto end;
            }

            m_openxr_backend_info.device_name_vec = {xr_system_props.systemName};
            m_openxr_backend_info.is_supported    = true;
        }

        /* 6. Cache refresh rate, if a relevant extension is available */
        if (is_xr_fb_display_refresh_rate_available)
        {
            PFN_xrGetDisplayRefreshRateFB pfn_xr_get_display_refresh_rate_fb = nullptr;

            if (XR_SUCCEEDED(xrGetInstanceProcAddr(xr_instance,
                                                   "xrGetDisplayRefreshRateFB",
                                                   reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xr_get_display_refresh_rate_fb) )))
            {
                float refresh_rate_fp32 = FLT_MAX;

                if (XR_SUCCEEDED(pfn_xr_get_display_refresh_rate_fb(xr_session,
                                                                   &refresh_rate_fp32) ))
                {
                    m_openxr_backend_info.refresh_rate = static_cast<uint32_t>(refresh_rate_fp32);
                }
            }
        }

        /* 7. Now determine per-eye texture resolution we want to use. */
        {
            uint32_t                             n_views                        = 0;
            std::vector<XrViewConfigurationView> xr_view_configuration_view_vec;

            if (!XR_SUCCEEDED(::xrEnumerateViewConfigurationViews(xr_instance,
                                                                  xr_system_id,
                                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                                  0, /* viewCapacityInput */
                                                                 &n_views,
                                                                  nullptr) ))
            {
                goto end;
            }

            xr_view_configuration_view_vec.resize(n_views,
                                                  {XR_TYPE_VIEW_CONFIGURATION_VIEW});

            if (!XR_SUCCEEDED(::xrEnumerateViewConfigurationViews(xr_instance,
                                                                  xr_system_id,
                                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                                  n_views,
                                                                 &n_views,
                                                                  xr_view_configuration_view_vec.data() )))
            {
                goto end;
            }

            m_openxr_backend_info.eye_texture_extents.at(0) = xr_view_configuration_view_vec.at(0).recommendedImageRectWidth;
            m_openxr_backend_info.eye_texture_extents.at(1) = xr_view_configuration_view_vec.at(1).recommendedImageRectHeight;
        }
    }

    /* Clean up */
    if (xr_session != XR_NULL_HANDLE)
    {
        ::xrDestroySession(xr_session);
    }

    if (xr_instance != XR_NULL_HANDLE)
    {
        ::xrDestroyInstance(xr_instance);
    }
end:
    ;
}

bool Launcher::VRSupport::is_vr_backend_supported(const Common::VRBackend& in_backend)
{
    bool result = false;

    switch (in_backend)
    {
        case Common::VRBackend::LIBOVR: result = m_libovr_backend_info.is_supported; break;
        case Common::VRBackend::OPENXR: result = m_openxr_backend_info.is_supported; break;

        default:
        {
            assert(false);
        }
    }

    return result;
}