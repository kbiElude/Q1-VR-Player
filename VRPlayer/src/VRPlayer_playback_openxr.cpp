/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */


#include "common_defines.inl"
#include "Extras/OVR_Math.h"
#include "OpenGL/globals.h"
#include "VRPlayer.h"
#include "VRPlayer_playback_openxr.h"
#include "VRPlayer_settings.h"




#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif


PlaybackOpenXR::PlaybackOpenXR(const float&    in_horizontal_fov_degrees,
                               const float&    in_aspect_ratio,
                               const Settings* in_settings_ptr)
    :m_aspect_ratio                          (in_aspect_ratio),
     m_gl_blit_src_texture_fb                (0),
     m_gl_preview_texture_fb                 (0),
     m_gl_preview_texture_id                 (0),
     m_horizontal_fov_degrees                (in_horizontal_fov_degrees),
     m_pfn_xr_get_opengl_gfx_requirements_khr(nullptr),
     m_pitch_angle                           (0),
     m_settings_ptr                          (in_settings_ptr),
     m_yaw_angle                             (0),
     m_xr_instance                           (0),
     m_xr_session                            (0),
     m_xr_session_state                      (XR_SESSION_STATE_UNKNOWN),
     m_xr_space                              (0),
     m_xr_system_id                          (0)
{
    /* Stub */
}

PlaybackOpenXR::~PlaybackOpenXR()
{
    /* Stub */
}

bool PlaybackOpenXR::acquire_eye_texture(const bool& in_left_eye,
                                         uint32_t*   out_eye_color_texture_id_ptr,
                                         uint32_t*   out_eye_color_texture_n_layer_ptr,
                                         uint32_t*   out_ui_color_texture_id_ptr,
                                         uint32_t*   out_ui_color_texture_n_layer_ptr)
{
    /* Q1 VR Player calls this function twice per "flip", once per each eye.
     *
     * This is in line with how OpenXR. Additionally, we perform frame start-related activities
     * when the app asks us to acquire the left eye's texture. This includes acquiring a swapchain
     * frame for each eye.
     * As for frame ending, we handle the event in present().
     */
    bool result = false;

    AI_ASSERT(!m_eye_props[0].is_active);
    AI_ASSERT(!m_eye_props[1].is_active);

    if (in_left_eye)
    {
        /* 0. Poll all available events from the runtime */
        do
        {
            XrEventDataBuffer event_data_buffer  = {XR_TYPE_EVENT_DATA_BUFFER};
            bool              is_event_available = false;

            is_event_available = ::xrPollEvent(m_xr_instance,
                                              &event_data_buffer) != XR_EVENT_UNAVAILABLE;

            if (!is_event_available)
            {
                break;
            }

            switch (event_data_buffer.type)
            {
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                {
                    auto session_state_changed_ptr = reinterpret_cast<const XrEventDataSessionStateChanged*>(&event_data_buffer);

                    m_xr_session_state = session_state_changed_ptr->state;
                    break;
                }

                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                {
                    /* XR instance is gone. Terminate the game. */
                    ::TerminateProcess(::GetCurrentProcess(),
                                       EXIT_FAILURE);

                    break;
                }

                case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                {
                    /* Nop. */
                    break;
                }

                default:
                {
                    /* Nop */
                }
            }
        }
        while (true);

        /* 1. Wait for the runtime to give us a green light.. */
        {
            XrFrameState    frame_state     = {XR_TYPE_FRAME_STATE};
            XrFrameWaitInfo frame_wait_info = {XR_TYPE_FRAME_WAIT_INFO};

            if (!XR_SUCCEEDED(::xrWaitFrame(m_xr_session,
                                           &frame_wait_info,
                                           &frame_state) ))
            {
                AI_ASSERT_FAIL();

                goto end;
            }

            m_current_frame_display_time  = frame_state.predictedDisplayTime;
            m_current_frame_should_render = frame_state.shouldRender;
        }

        /* 2. Begin the frame */
        {
            XrFrameBeginInfo frame_begin_info = {XR_TYPE_FRAME_BEGIN_INFO};

            if (!XR_SUCCEEDED(::xrBeginFrame(m_xr_session,
                                            &frame_begin_info) ))
            {
                AI_ASSERT_FAIL();

                goto end;
            }
        }

        /* 3. Extract frame-specific data. */
        {
            uint32_t         n_views             = 0;
            XrViewLocateInfo view_locate_info    = {XR_TYPE_VIEW_LOCATE_INFO};
            XrViewState      view_state          = {XR_TYPE_VIEW_STATE};
            XrView           view_data       [2] = {};

            view_locate_info.displayTime           = m_current_frame_display_time;
            view_locate_info.next                  = nullptr;
            view_locate_info.space                 = m_xr_space;
            view_locate_info.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

            view_data[0].type = XR_TYPE_VIEW;
            view_data[1].type = XR_TYPE_VIEW;

            if (!XR_SUCCEEDED(::xrLocateViews(m_xr_session,
                                             &view_locate_info,
                                             &view_state,
                                              2, /* viewCapacityInput */
                                             &n_views,
                                              view_data) ))
            {
                AI_ASSERT_FAIL();

                goto end;
            }

            AI_ASSERT(n_views == 2);

            /* 4. Update eye-specific data */
            const float eye_delta_x = fabs(view_data[1].pose.position.x - view_data[0].pose.position.x);

            for (uint32_t n_eye = 0;
                          n_eye < 2;
                        ++n_eye)
            {
                m_eye_props[n_eye].fov              = view_data[n_eye].fov;
                m_eye_props[n_eye].pose.orientation = view_data[n_eye].pose.orientation;
                m_eye_props[n_eye].pose.position.x  = (n_eye == 0) ? -eye_delta_x * 0.5f
                                                                   :  eye_delta_x * 0.5f;

                if (view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT)
                {
                    const OVR::Quatf    orientation = OVR::Quatf   (view_data[n_eye].pose.orientation.x,
                                                                    view_data[n_eye].pose.orientation.y,
                                                                    view_data[n_eye].pose.orientation.z,
                                                                    view_data[n_eye].pose.orientation.w);
                    const OVR::Vector3f position    = OVR::Vector3f(view_data[n_eye].pose.position.x,
                                                                    view_data[n_eye].pose.position.y,
                                                                    view_data[n_eye].pose.position.z);
                    OVR::Posef          pose        = OVR::Posef   (orientation,
                                                                    position);

                    const auto orientation_matrix = OVR::Matrix4f(pose);
                    float      temp;

                    orientation_matrix.ToEulerAngles<OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z, OVR::Rotate_CW, OVR::Handed_R>(&m_pitch_angle,
                                                                                                                           &m_yaw_angle,
                                                                                                                           &temp);

                    m_pitch_angle *= -m_horizontal_fov_degrees;
                    m_yaw_angle   *=  m_horizontal_fov_degrees;
                }
            }
        }

        m_eye_props[0].is_active = true;
    }
    else
    {
        m_eye_props[1].is_active = true;
    }

    if (m_current_frame_should_render)
    {
        const auto n_eye                  = (in_left_eye) ? 0u
                                                          : 1u;
        const auto uses_arrayed_swapchain = supports_separate_ui_texture();

        {
            XrSwapchainImageAcquireInfo swapchain_image_acquire_info = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

            if (!XR_SUCCEEDED(::xrAcquireSwapchainImage(m_eye_props[n_eye].xr_swapchain,
                                                       &swapchain_image_acquire_info,
                                                       &m_eye_props[n_eye].n_acquired_swapchain_image) ))
            { 
                AI_ASSERT_FAIL();

                goto end;
            }
        }

        /* 5. Wait for the requested eye's swapchain image to become available. */
        {
            const auto n_swapchain_image = m_eye_props[n_eye].n_acquired_swapchain_image;

            AI_ASSERT(m_eye_props[n_eye].swapchain_texture_gl_id_vec.size() > n_swapchain_image);

            {
                XrSwapchainImageWaitInfo wait_info = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};

                wait_info.timeout = XR_INFINITE_DURATION;

                if (!XR_SUCCEEDED(::xrWaitSwapchainImage(m_eye_props[n_eye].xr_swapchain,
                                                        &wait_info) ))
                {
                    AI_ASSERT_FAIL();

                    goto end;
                }
            }

            *out_eye_color_texture_id_ptr      = m_eye_props[n_eye].swapchain_texture_gl_id_vec.at(n_swapchain_image);
            *out_eye_color_texture_n_layer_ptr = 0;
            *out_ui_color_texture_id_ptr       = (n_eye == 0 && uses_arrayed_swapchain) ? *out_eye_color_texture_id_ptr
                                                                                        : UINT32_MAX;
            *out_ui_color_texture_n_layer_ptr  = (n_eye == 0 && uses_arrayed_swapchain) ? 1
                                                                                        : UINT32_MAX;
        }
    }
    else
    {
        *out_eye_color_texture_id_ptr      = UINT32_MAX;
        *out_eye_color_texture_n_layer_ptr = UINT32_MAX;
        *out_ui_color_texture_id_ptr       = UINT32_MAX;
        *out_ui_color_texture_n_layer_ptr  = UINT32_MAX;
    }

    /* 6. Done */
    result = true;
end:
    return result;
}

bool PlaybackOpenXR::commit_eye_texture()
{
    bool result = false;

    AI_ASSERT(m_eye_props[0].is_active != m_eye_props[1].is_active);

    if (m_current_frame_should_render)
    {
        const auto is_left_eye   = m_eye_props[0].is_active;
        auto       eye_props_ptr = (is_left_eye) ? m_eye_props + 0
                                                 : m_eye_props + 1;

        XrSwapchain                 swapchain                    = eye_props_ptr->xr_swapchain;
        XrSwapchainImageReleaseInfo swapchain_image_release_info = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};

        /* Blit contents to the mirror texture before releasing the image */
        {
            const auto     preview_texture_width     = eye_props_ptr->eye_texture_extents.at(0) / VRPlayer::EYE_TO_PREVIEW_TEXTURE_DIVISOR;
            const auto     preview_texture_height    = eye_props_ptr->eye_texture_extents.at(1) / VRPlayer::EYE_TO_PREVIEW_TEXTURE_DIVISOR;
            const auto     uses_arrayed_swapchain    = supports_separate_ui_texture();
            const uint32_t x1y1                  [2] = {(is_left_eye) ? 0                     : preview_texture_width,     0};
            const uint32_t x2y2                  [2] = {(is_left_eye) ? preview_texture_width : preview_texture_width * 2, preview_texture_height};

            auto pfn_gl_bind_framebuffer          = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>        (OpenGL::g_cached_gl_bind_framebuffer);
            auto pfn_gl_blit_framebuffer          = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>        (OpenGL::g_cached_gl_blit_framebuffer);
            auto pfn_gl_enable                    = reinterpret_cast<PFNGLENABLEPROC>                 (OpenGL::g_cached_gl_enable);
            auto pfn_gl_framebuffer_texture_2d    = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>   (OpenGL::g_cached_gl_framebuffer_texture_2D);
            auto pfn_gl_framebuffer_texture_layer = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURELAYERPROC>(OpenGL::g_cached_gl_framebuffer_texture_layer);

            pfn_gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER,
                                    m_gl_preview_texture_fb);
            pfn_gl_bind_framebuffer(GL_READ_FRAMEBUFFER,
                                    m_gl_blit_src_texture_fb);

            if (is_left_eye && uses_arrayed_swapchain)
            {
                pfn_gl_framebuffer_texture_layer(GL_READ_FRAMEBUFFER,
                                                 GL_COLOR_ATTACHMENT0,
                                                 eye_props_ptr->swapchain_texture_gl_id_vec.at(eye_props_ptr->n_acquired_swapchain_image),
                                                 0,  /* level */
                                                 0); /* layer */
            }
            else
            {
                /* Note, for right eye, we always use a single-layered 2D texture-based swapchain */
                pfn_gl_framebuffer_texture_2d(GL_READ_FRAMEBUFFER,
                                              GL_COLOR_ATTACHMENT0,
                                              GL_TEXTURE_2D,
                                              eye_props_ptr->swapchain_texture_gl_id_vec.at(eye_props_ptr->n_acquired_swapchain_image),
                                              0); /* level */
            }

            pfn_gl_enable          (GL_FRAMEBUFFER_SRGB);
            pfn_gl_blit_framebuffer(0,                                        /* srcX0 */
                                    eye_props_ptr->eye_texture_extents.at(1), /* srcY0 */
                                    eye_props_ptr->eye_texture_extents.at(0), /* srcX1 */
                                    0,                                        /* srcY1 */
                                    x1y1[0],                                  /* dstX0 */
                                    x1y1[1],                                  /* dstY0 */
                                    x2y2[0],                                  /* dstX1 */
                                    x2y2[1],                                  /* dstY1 */
                                    GL_COLOR_BUFFER_BIT,
                                    GL_NEAREST);

            pfn_gl_bind_framebuffer(GL_FRAMEBUFFER,
                                    0);
        }

        /* Proceed. */
        if (!XR_SUCCEEDED(::xrReleaseSwapchainImage(swapchain,
                                                   &swapchain_image_release_info) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    m_eye_props[0].is_active = false;
    m_eye_props[1].is_active = false;
    result                   = true;
end:
    return result;
}

VRPlaybackUniquePtr PlaybackOpenXR::create(const float&    in_horizontal_fov_degrees,
                                           const float&    in_aspect_ratio,
                                           const Settings* in_settings_ptr)
{
    VRPlaybackUniquePtr result_ptr;

    result_ptr.reset(new PlaybackOpenXR(in_horizontal_fov_degrees,
                                        in_aspect_ratio,
                                        in_settings_ptr) );

    AI_ASSERT(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!dynamic_cast<PlaybackOpenXR*>(result_ptr.get() )->init())
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void PlaybackOpenXR::deinit_for_bound_gl_context()
{
    auto pfn_gl_delete_framebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(OpenGL::g_cached_gl_delete_framebuffers);
    auto pfn_gl_delete_textures     = reinterpret_cast<PFNGLDELETETEXTURESPROC>    (OpenGL::g_cached_gl_delete_textures);

    if (m_gl_blit_src_texture_fb != 0)
    {
        pfn_gl_delete_framebuffers(1, /* n */
                                  &m_gl_blit_src_texture_fb);

        m_gl_blit_src_texture_fb = 0;
    }

    if (m_gl_preview_texture_fb != 0)
    {
        pfn_gl_delete_framebuffers(1, /* n */
                                  &m_gl_preview_texture_fb);

        m_gl_preview_texture_fb = 0;
    }

    if (m_gl_preview_texture_id != 0)
    {
        pfn_gl_delete_textures(1, /* n */
                              &m_gl_preview_texture_id);

        m_gl_preview_texture_id = 0;
    }

    if (m_xr_space != 0)
    {
        ::xrDestroySpace(m_xr_space);

        m_xr_space = 0;
    }

    if (m_xr_session != 0)
    {
        ::xrEndSession(m_xr_session);

        m_xr_session = 0;
    }

    if (m_xr_instance != 0)
    {
        ::xrDestroyInstance(m_xr_instance);

        m_xr_instance = 0;
    }
}

float PlaybackOpenXR::get_current_pitch_angle() const
{
    return m_pitch_angle;
}

float PlaybackOpenXR::get_current_yaw_angle() const
{
    return m_yaw_angle;
}

float PlaybackOpenXR::get_eye_offset_x(const bool& in_left_eye) const
{
    auto result = (in_left_eye) ? m_eye_props[0].pose.position.x
                                : m_eye_props[1].pose.position.x;

    return result;
}

std::array<uint32_t, 2> PlaybackOpenXR::get_eye_texture_resolution(const bool& in_left_eye) const
{
    return (in_left_eye) ? m_eye_props[0].eye_texture_extents
                         : m_eye_props[1].eye_texture_extents;
}

uint32_t PlaybackOpenXR::get_preview_texture_gl_id() const
{
    return m_gl_preview_texture_id;
}

float PlaybackOpenXR::get_tan_between_view_vec_and_bottom_fov_edge(const bool& in_left_eye) const
{
    return (in_left_eye) ? -tanf(m_eye_props[0].fov.angleDown)
                         : -tanf(m_eye_props[1].fov.angleDown);
}

float PlaybackOpenXR::get_tan_between_view_vec_and_top_fov_edge(const bool& in_left_eye) const
{
    return (in_left_eye) ? tanf(m_eye_props[0].fov.angleUp)
                         : tanf(m_eye_props[1].fov.angleUp);
}

bool PlaybackOpenXR::init()
{
    static const char* xr_khr_opengl_extension_string = "XR_KHR_opengl_enable";

    bool                               result            = false;
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

    /* While we don't have access to an OpenGL context at this point, we can still perform a few OpenXRL initialization steps.*/
    {
        uint32_t n_extensions_available = 0;

        if (!XR_SUCCEEDED(::xrEnumerateInstanceExtensionProperties(nullptr,                 /* layerName             */
                                                                   0,                       /* propertyCapacityInput */
                                                                   &n_extensions_available,
                                                                   nullptr) ))              /* properties            */
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate available OpenXR extensions. No VR system installed?",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        xr_ext_props_vec.resize(n_extensions_available,
                                XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES, nullptr});

        if (!XR_SUCCEEDED(::xrEnumerateInstanceExtensionProperties(nullptr,                    /* layerName             */
                                                                   n_extensions_available,     /* propertyCapacityInput */
                                                                  &n_extensions_available,
                                                                   xr_ext_props_vec.data()) )) /* properties            */
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate available OpenXR extensions. No VR system installed?",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    if (!is_ext_available_func(xr_khr_opengl_extension_string) )
    {
        ::MessageBox(HWND_DESKTOP,
                     "OpenXR extension XR_KHR_opengl_enable is not available.",
                     "Error!",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    {
        XrInstanceCreateInfo create_info = {};

        create_info.applicationInfo.apiVersion         = XR_API_VERSION_1_0;
        create_info.applicationInfo.applicationVersion = 1;
        create_info.applicationInfo.engineVersion      = 0;
        create_info.createFlags                        = 0;
        create_info.enabledApiLayerCount               = 0;
        create_info.enabledApiLayerNames               = nullptr;
        create_info.enabledExtensionCount              = 1;
        create_info.enabledExtensionNames              = &xr_khr_opengl_extension_string;
        create_info.type                               = XR_TYPE_INSTANCE_CREATE_INFO;

        strcpy(create_info.applicationInfo.applicationName,
               "Q1 VR Player");
        strcpy(create_info.applicationInfo.engineName,
               "Q1 VR Player");

        if (!XR_SUCCEEDED(::xrCreateInstance(&create_info,
                                             &m_xr_instance) ))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not create an OpenXR instance.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    /* 2. Is there a head-mounted display system available? */
    {
        XrSystemGetInfo get_info = {};

        get_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        get_info.next       = nullptr;
        get_info.type       = XR_TYPE_SYSTEM_GET_INFO;

        if (!XR_SUCCEEDED(::xrGetSystem(m_xr_instance,
                                       &get_info,
                                       &m_xr_system_id) ))
        {
            goto end;
        }
    }

    /* 3. OpenXR requires us to call xrGetOpenGLGraphicsRequirementsKHR() before we open an XR session. */
    {
        XrGraphicsRequirementsOpenGLKHR xr_gfx_requirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};

        if (!XR_SUCCEEDED(xrGetInstanceProcAddr(m_xr_instance,
                                                "xrGetOpenGLGraphicsRequirementsKHR",
                                                reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xr_get_opengl_gfx_requirements_khr) )))
        {
            goto end;
        }

        if (!XR_SUCCEEDED(m_pfn_xr_get_opengl_gfx_requirements_khr(m_xr_instance,
                                                                   m_xr_system_id,
                                                                  &xr_gfx_requirements) ))
        {
            goto end;
        }
    }

    /* 4. Confirm opaque environment blend mode is supported. */
    {
        uint32_t                            n_environment_blend_modes = 0;
        std::vector<XrEnvironmentBlendMode> xr_env_blend_mode_vec;

        if (!XR_SUCCEEDED(::xrEnumerateEnvironmentBlendModes(m_xr_instance,
                                                             m_xr_system_id,
                                                             XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                             0, /* environmentBlendModeCapacityInput */
                                                            &n_environment_blend_modes,
                                                             nullptr) )) /* environmentBlendModes */
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        xr_env_blend_mode_vec.resize(n_environment_blend_modes);

        if (!XR_SUCCEEDED(::xrEnumerateEnvironmentBlendModes(m_xr_instance,
                                                             m_xr_system_id,
                                                             XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                             n_environment_blend_modes,
                                                            &n_environment_blend_modes,
                                                             xr_env_blend_mode_vec.data() ))) /* environmentBlendModes */
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        if (std::find(xr_env_blend_mode_vec.begin(),
                      xr_env_blend_mode_vec.end  (),
                      XR_ENVIRONMENT_BLEND_MODE_OPAQUE) == xr_env_blend_mode_vec.end() )
        {
            ::MessageBox(HWND_DESKTOP,
                         "Opaque environment blend mode is not supported by your VR runtime.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    /* 5. All done for now. The remaining initialization is going to be carried out in setup_for_bound_gl_context() */
    result = true;
end:
    return result;
}

bool PlaybackOpenXR::present()
{
    bool       result                 = false;
    const auto uses_arrayed_swapchain = supports_separate_ui_texture();

    AI_ASSERT(!m_eye_props[0].is_active);
    AI_ASSERT(!m_eye_props[1].is_active);

    {
        XrFrameEndInfo                      frame_end_info                  = {XR_TYPE_FRAME_END_INFO};
        XrCompositionLayerProjection        frame_layer_projection          = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        XrCompositionLayerProjectionView    frame_layer_projection_views[2] = {};
        XrCompositionLayerQuad              frame_layer_ui                  = {};
        const XrCompositionLayerBaseHeader* frame_layer_ptrs            [3] = {reinterpret_cast<const XrCompositionLayerBaseHeader*>(&frame_layer_projection),
                                                                               reinterpret_cast<const XrCompositionLayerBaseHeader*>(&frame_layer_ui)        };

        frame_layer_projection_views[0].fov                              = m_eye_props[0].fov;
        frame_layer_projection_views[0].pose                             = m_eye_props[0].pose;
        frame_layer_projection_views[0].subImage.imageArrayIndex         = 0;
        frame_layer_projection_views[0].subImage.imageRect.extent.height = m_eye_props[0].eye_texture_extents.at(1);
        frame_layer_projection_views[0].subImage.imageRect.extent.width  = m_eye_props[0].eye_texture_extents.at(0);
        frame_layer_projection_views[0].subImage.swapchain               = m_eye_props[0].xr_swapchain;
        frame_layer_projection_views[0].type                             = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;

        frame_layer_projection_views[1].fov                              = m_eye_props[1].fov;
        frame_layer_projection_views[1].pose                             = m_eye_props[1].pose;
        frame_layer_projection_views[1].subImage.imageArrayIndex         = 0;
        frame_layer_projection_views[1].subImage.imageRect.extent.height = m_eye_props[1].eye_texture_extents.at(1);
        frame_layer_projection_views[1].subImage.imageRect.extent.width  = m_eye_props[1].eye_texture_extents.at(0);
        frame_layer_projection_views[1].subImage.swapchain               = m_eye_props[1].xr_swapchain;
        frame_layer_projection_views[1].type                             = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;

        /* Make sure to use HFOV settings as specified by the class user. */
        frame_layer_projection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        frame_layer_projection.space      = m_xr_space;
        frame_layer_projection.viewCount  = 2;
        frame_layer_projection.views      = frame_layer_projection_views;

        frame_layer_ui.eyeVisibility                    = XR_EYE_VISIBILITY_BOTH;
        frame_layer_ui.layerFlags                       = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        frame_layer_ui.next                             = nullptr;
        frame_layer_ui.pose.orientation.x               = 0;
        frame_layer_ui.pose.orientation.y               = 0;
        frame_layer_ui.pose.orientation.z               = 0;
        frame_layer_ui.pose.orientation.w               = 1;
        frame_layer_ui.pose.position.x                  = 0;
        frame_layer_ui.pose.position.z                  = 0;
        frame_layer_ui.pose.position.x                  = 0;
        frame_layer_ui.size.height                      = 1;
        frame_layer_ui.size.width                       = 1;
        frame_layer_ui.space                            = m_xr_space;
        frame_layer_ui.subImage.imageArrayIndex         = 1;
        frame_layer_ui.subImage.imageRect.extent.height = m_eye_props[0].eye_texture_extents.at(1);
        frame_layer_ui.subImage.imageRect.extent.width  = m_eye_props[0].eye_texture_extents.at(0);
        frame_layer_ui.subImage.imageRect.offset.x      = 0;
        frame_layer_ui.subImage.imageRect.offset.y      = 0;
        frame_layer_ui.subImage.swapchain               = m_eye_props[0].xr_swapchain;
        frame_layer_ui.type                             = XR_TYPE_COMPOSITION_LAYER_QUAD;

        frame_end_info.displayTime          = m_current_frame_display_time;
        frame_end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        frame_end_info.layerCount           = (m_current_frame_should_render) ? (uses_arrayed_swapchain) ? 2u
                                                                                                         : 1u
                                                                              : 0u;
        frame_end_info.layers               = frame_layer_ptrs;

        if (!XR_SUCCEEDED(::xrEndFrame(m_xr_session,
                                      &frame_end_info) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    m_eye_props[0].is_active = false;
    m_eye_props[1].is_active = false;

    result = true;
end:
    return result;
}

bool PlaybackOpenXR::setup_for_bound_gl_context(const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2,
                                                HDC                            in_window_dc,
                                                HGLRC                          in_glrc)
{
    bool     result           = false;
    uint64_t swapchain_format = 0;

    assert(in_glrc      != 0);
    assert(in_window_dc != 0);
    assert(m_xr_session == 0);

    /* 1. Go ahead and create an XR session. */
    {
        XrSessionCreateInfo             create_info{};
        XrGraphicsBindingOpenGLWin32KHR gfx_binding_gl_win32 = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};

        gfx_binding_gl_win32.hDC   = in_window_dc;
        gfx_binding_gl_win32.hGLRC = in_glrc;

        create_info.createFlags =  0;
        create_info.next        = &gfx_binding_gl_win32;
        create_info.systemId    =  m_xr_system_id;
        create_info.type        =  XR_TYPE_SESSION_CREATE_INFO;

        if (!XR_SUCCEEDED(::xrCreateSession(m_xr_instance,
                                           &create_info,
                                           &m_xr_session)) )
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not create an OpenXR session.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    /* 2. Determine eye texture resolutions */
    {
        uint32_t                             n_views                        = 0;
        std::vector<XrViewConfigurationView> xr_view_configuration_view_vec;

        if (!XR_SUCCEEDED(::xrEnumerateViewConfigurationViews(m_xr_instance,
                                                              m_xr_system_id,
                                                              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                              0, /* viewCapacityInput */
                                                             &n_views,
                                                              nullptr) ))
        {
            goto end;
        }

        xr_view_configuration_view_vec.resize(n_views,
                                              {XR_TYPE_VIEW_CONFIGURATION_VIEW});

        if (!XR_SUCCEEDED(::xrEnumerateViewConfigurationViews(m_xr_instance,
                                                              m_xr_system_id,
                                                              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                              n_views,
                                                             &n_views,
                                                              xr_view_configuration_view_vec.data() )))
        {
            goto end;
        }

        if (n_views != 2)
        {
            ::MessageBox(HWND_DESKTOP,
                         "Invalid number of views returned for stereo view.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        for (uint32_t n_eye = 0;
                      n_eye < 2; /* left, right */
                    ++n_eye)
        {
            m_eye_props[n_eye].eye_texture_extents.at(0) = xr_view_configuration_view_vec.at(n_eye).recommendedImageRectWidth;
            m_eye_props[n_eye].eye_texture_extents.at(1) = xr_view_configuration_view_vec.at(n_eye).recommendedImageRectHeight;
        }
    }

    /* 3. Determine swapchain image format to use */
    {
        uint32_t             n_xr_formats     = 0;
        std::vector<int64_t> xr_format_vec;

        if (!XR_SUCCEEDED(::xrEnumerateSwapchainFormats(m_xr_session,
                                                        0, /* formatCapacityInput */
                                                       &n_xr_formats,
                                                        nullptr) ))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate available swapchain formats.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        xr_format_vec.resize(n_xr_formats);

        if (!XR_SUCCEEDED(::xrEnumerateSwapchainFormats(m_xr_session,
                                                        n_xr_formats,
                                                       &n_xr_formats,
                                                        xr_format_vec.data() )))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate available swapchain formats.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        /* For 3D rendering, we favor RGBA8 format. It is NOT guaranteed to be supported, sadly, so if unavailable hope the first one reported will do the job.. */
        if (std::find(xr_format_vec.begin(),
                      xr_format_vec.end  (),
                      GL_SRGB8_ALPHA8) == xr_format_vec.end() )
        {
            ::MessageBox(HWND_DESKTOP,
                         "Your VR system does not support SRGB8_ALPHA8 swapchains.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        swapchain_format = GL_SRGB8_ALPHA8;
    }

    /* 4. Go ahead and create the swapchains, one for each eye. */
    {
        XrSwapchainCreateInfo swapchain_create_info  = {};
        const auto            uses_arrayed_swapchain = supports_separate_ui_texture();

        swapchain_create_info.createFlags = 0;
        swapchain_create_info.faceCount   = 1;
        swapchain_create_info.format      = swapchain_format;
        swapchain_create_info.height      = std::max(m_eye_props[0].eye_texture_extents.at(1),
                                                     m_eye_props[1].eye_texture_extents.at(1) );
        swapchain_create_info.mipCount    = 1;
        swapchain_create_info.next        = nullptr;
        swapchain_create_info.sampleCount = 1;
        swapchain_create_info.type        = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapchain_create_info.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.width       = std::max(m_eye_props[0].eye_texture_extents.at(0),
                                                     m_eye_props[1].eye_texture_extents.at(0) );

        for (uint32_t n_eye = 0;
                      n_eye < 2;
                    ++n_eye)
        {
            auto xr_swapchain_ptr = &m_eye_props[n_eye].xr_swapchain;

            swapchain_create_info.arraySize = (n_eye == 0 && uses_arrayed_swapchain) ? 2  /* 3D projection + UI */
                                                                                     : 1; /* 3D projection      */

            if (!XR_SUCCEEDED(::xrCreateSwapchain(m_xr_session,
                                                 &swapchain_create_info,
                                                  xr_swapchain_ptr) ))
            {
                ::MessageBox(HWND_DESKTOP,
                             "Could not create an OpenXR swapchain.",
                             "Error!",
                             MB_OK | MB_ICONERROR);

                goto end;
            }
        }
    }

    /* 5. Cache swapchain image IDs */
    for (uint32_t n_eye = 0;
                  n_eye < 2;
                ++n_eye)
    {
        auto                                   eye_props_ptr             = m_eye_props + n_eye;
        uint32_t                               n_swapchain_images        = 0;
        std::vector<XrSwapchainImageOpenGLKHR> xr_swapchain_image_gl_vec;

        if (!XR_SUCCEEDED(::xrEnumerateSwapchainImages(eye_props_ptr->xr_swapchain,
                                                       0,                   /* imageCapacityInput */
                                                      &n_swapchain_images,
                                                       nullptr) ))          /* images             */
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate swapchain images.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        assert(n_swapchain_images != 0);

        eye_props_ptr->swapchain_texture_gl_id_vec.resize(n_swapchain_images);
        xr_swapchain_image_gl_vec.resize                 (n_swapchain_images,
                                                          XrSwapchainImageOpenGLKHR{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});

        if (!XR_SUCCEEDED(::xrEnumerateSwapchainImages(eye_props_ptr->xr_swapchain,
                                                       n_swapchain_images,
                                                      &n_swapchain_images,
                                                       reinterpret_cast<XrSwapchainImageBaseHeader*>(xr_swapchain_image_gl_vec.data() ))))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not enumerate swapchain images.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }

        for (uint32_t n_swapchain_image = 0;
                      n_swapchain_image < n_swapchain_images;
                    ++n_swapchain_image)
        {
            eye_props_ptr->swapchain_texture_gl_id_vec.at(n_swapchain_image) = xr_swapchain_image_gl_vec.at(n_swapchain_image).image;
        }
    }

    /* 6. Create helper objects we're going to need to provide preview texture */
    {
        auto pfn_gl_bind_framebuffer       = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_bind_framebuffer);
        auto pfn_gl_bind_texture           = reinterpret_cast<PFNGLBINDTEXTUREPROC>         (OpenGL::g_cached_gl_bind_texture);
        auto pfn_gl_framebuffer_texture_2d = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(OpenGL::g_cached_gl_framebuffer_texture_2D);
        auto pfn_gl_gen_framebuffers       = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>     (OpenGL::g_cached_gl_gen_framebuffers);
        auto pfn_gl_gen_textures           = reinterpret_cast<PFNGLGENTEXTURESPROC>         (OpenGL::g_cached_gl_gen_textures);
        auto pfn_gl_tex_image_2d           = reinterpret_cast<PFNGLTEXIMAGE2DPROC>          (OpenGL::g_cached_gl_tex_image_2D);
        auto pfn_gl_tex_parameteri         = reinterpret_cast<PFNGLTEXPARAMETERIPROC>       (OpenGL::g_cached_gl_tex_parameteri);

        pfn_gl_gen_framebuffers(1, /* n */
                               &m_gl_blit_src_texture_fb);
        pfn_gl_gen_framebuffers(1, /* n */
                               &m_gl_preview_texture_fb);

        pfn_gl_gen_textures(1, /* n */
                           &m_gl_preview_texture_id);

        pfn_gl_bind_texture  (GL_TEXTURE_2D,
                              m_gl_preview_texture_id);
        pfn_gl_tex_image_2d  (GL_TEXTURE_2D,
                              0,                  /* level  */
                              GL_RGB,
                              m_eye_props[0].eye_texture_extents.at(0) / VRPlayer::EYE_TO_PREVIEW_TEXTURE_DIVISOR * 2,
                              m_eye_props[0].eye_texture_extents.at(1) / VRPlayer::EYE_TO_PREVIEW_TEXTURE_DIVISOR * 2,
                              0,                  /* border */
                              GL_RGB,
                              GL_UNSIGNED_BYTE,
                              nullptr);           /* pixels */
        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_MAG_FILTER,
                              GL_LINEAR);
        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_MIN_FILTER,
                              GL_LINEAR);

        pfn_gl_bind_framebuffer      (GL_DRAW_FRAMEBUFFER,
                                      m_gl_preview_texture_fb);
        pfn_gl_framebuffer_texture_2d(GL_DRAW_FRAMEBUFFER,
                                      GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D,
                                      m_gl_preview_texture_id,
                                      0); /* level */
    }

    /* 7. Wait until we're good to start the XR session */
    {
        do
        {
            XrEventDataBuffer event_data_buffer = {XR_TYPE_EVENT_DATA_BUFFER};

            if (!XR_SUCCEEDED(::xrPollEvent(m_xr_instance,
                                           &event_data_buffer) ))
            {
                assert(false);

                goto end;
            }

            if (event_data_buffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
            {
                auto session_state_changed_ptr = reinterpret_cast<const XrEventDataSessionStateChanged*>(&event_data_buffer);

                if (session_state_changed_ptr->state == XR_SESSION_STATE_READY)
                {
                    m_xr_session_state = session_state_changed_ptr->state;

                    break;
                }
            }
        }
        while (true);
    }

    /* 8. Start the session. */
    {
        XrSessionBeginInfo session_begin_info;

        session_begin_info.next                         = nullptr;
        session_begin_info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        session_begin_info.type                         = XR_TYPE_SESSION_BEGIN_INFO;

        if (!XR_SUCCEEDED(::xrBeginSession(m_xr_session,
                                          &session_begin_info) ))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not begin an OpenXR session.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    /* 9. Create a local space */
    {
        XrReferenceSpaceCreateInfo space_create_info = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};

        space_create_info.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f}; /* no rotation    */
        space_create_info.poseInReferenceSpace.position    = {0.0f, 0.0f, 0.0f};       /* no translation */
        space_create_info.referenceSpaceType               = XR_REFERENCE_SPACE_TYPE_LOCAL;

        if (!XR_SUCCEEDED(::xrCreateReferenceSpace(m_xr_session,
                                                  &space_create_info,
                                                  &m_xr_space) ))
        {
            ::MessageBox(HWND_DESKTOP,
                         "Could not create a local XR space.",
                         "Error!",
                         MB_OK | MB_ICONERROR);

            goto end;
        }
    }

    /* Done. */
    result = true;
end:
    return result;
}