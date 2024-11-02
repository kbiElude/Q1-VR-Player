/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_defines.inl"
#include "Extras/OVR_Math.h"
#include "OpenGL/globals.h"
#include "VRPlayer_playback_ovr.h"
#include "VRPlayer_settings.h"

#define UI_SWAPCHAIN_TEXTURE_ID (65534)


PlaybackOVR::PlaybackOVR(const float&    in_horizontal_fov_degrees,
                         const float&    in_aspect_ratio,
                         const Settings* in_settings_ptr)
    :m_aspect_ratio                    (in_aspect_ratio),
     m_eye_texture_acquisition_state   (AcquisitionState::NONE),
     m_fov_ports                       {},
     m_graphics_luid                   {},
     m_hmd_descriptor                  {},
     m_horizontal_fov_degrees          (in_horizontal_fov_degrees),
     m_left_eye_fov_texture_resolution {},
     m_mirror_texture                  {},
     m_mirror_texture_id               (0),
     m_pitch_angle                     {},
     m_n_frames_presented              (0),
     m_preview_texture_extents_u32vec2 {},
     m_right_eye_fov_texture_resolution{},
     m_sensor_sample_time              (DBL_MAX),
     m_session                         {},
     m_settings_ptr                    (in_settings_ptr),
     m_ui_swapchain                    (nullptr),
     m_yaw_angle                       {}
{
    /* Stub */
}

PlaybackOVR::~PlaybackOVR()
{
    /* Stub */
}

bool PlaybackOVR::acquire_eye_texture(const bool& in_left_eye,
                                      uint32_t*   out_eye_color_texture_id_ptr,
                                      uint32_t*   out_eye_color_texture_n_layer_ptr,
                                      uint32_t*   out_ui_color_texture_id_ptr,
                                      uint32_t*   out_ui_color_texture_n_layer_ptr)
{
    bool result = false;

    AI_ASSERT(m_eye_texture_acquisition_state == AcquisitionState::NONE);

    /* 3D swapchain */
    {
        int         n_swapchain_index    = UINT32_MAX;
        const auto& current_eye_gl_props = (in_left_eye) ? m_left_eye_gl_props
                                                         : m_right_eye_gl_props;

        if (!OVR_SUCCESS(::ovr_GetTextureSwapChainCurrentIndex(m_session,
                                                               current_eye_gl_props.color_texture_swapchain,
                                                              &n_swapchain_index) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        if (!OVR_SUCCESS(::ovr_GetTextureSwapChainBufferGL(m_session,
                                                           current_eye_gl_props.color_texture_swapchain,
                                                           n_swapchain_index,
                                                           out_eye_color_texture_id_ptr) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        *out_eye_color_texture_n_layer_ptr = 0;
    }

    /* UI swapchain */
    if (in_left_eye)
    {
        *out_ui_color_texture_id_ptr      = UI_SWAPCHAIN_TEXTURE_ID;
        *out_ui_color_texture_n_layer_ptr = 0;
    }
    else
    {
        *out_ui_color_texture_id_ptr      = UINT32_MAX;
        *out_ui_color_texture_n_layer_ptr = UINT32_MAX;
    }


    if (in_left_eye)
    {
        ::ovr_GetEyePoses(m_session,
                          m_n_frames_presented,
                          ovrTrue, /* latencyMarker */
                          m_hmd_to_eye_pose,
                          m_eye_poses,
                         &m_sensor_sample_time);
    }

    {
        const auto n_eye_pose         = (in_left_eye) ? 0u
                                                      : 1u;
        const auto orientation_matrix = OVR::Matrix4f(m_eye_poses[n_eye_pose].Orientation);

        float temp;

        orientation_matrix.ToEulerAngles<OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z, OVR::Rotate_CW, OVR::Handed_R>(&m_pitch_angle,
                                                                                                               &m_yaw_angle,
                                                                                                               &temp);

        m_pitch_angle *= -m_horizontal_fov_degrees;
        m_yaw_angle   *=  m_horizontal_fov_degrees;
    }

    m_eye_texture_acquisition_state = (in_left_eye) ? AcquisitionState::LEFT_EYE
                                                    : AcquisitionState::RIGHT_EYE;
    result                          = true;
end:
    return result;
}

bool PlaybackOVR::commit_eye_texture()
{
    bool result = false;

    AI_ASSERT( (m_eye_texture_acquisition_state == AcquisitionState::LEFT_EYE)  ||
               (m_eye_texture_acquisition_state == AcquisitionState::RIGHT_EYE) );

    if (!OVR_SUCCESS(::ovr_CommitTextureSwapChain(m_session,
                                                  (m_eye_texture_acquisition_state == AcquisitionState::LEFT_EYE) ? m_left_eye_gl_props.color_texture_swapchain
                                                                                                                  : m_right_eye_gl_props.color_texture_swapchain) ))
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    if (m_eye_texture_acquisition_state == AcquisitionState::RIGHT_EYE)
    {
        /* UI contents has been rendered to a helper texture. Blit it to the swapchain. */
        auto pfn_gl_bind_framebuffer      = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_bind_framebuffer);
        auto pfn_gl_blit_framebuffer      = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_blit_framebuffer);
        auto pfn_gl_framebuffer_texture2d = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(OpenGL::g_cached_gl_framebuffer_texture_2D);

        int    n_swapchain_index              = UINT32_MAX;
        GLuint libovr_ui_swapchain_texture_id = 0;

        if (!OVR_SUCCESS(::ovr_GetTextureSwapChainCurrentIndex(m_session,
                                                               m_ui_swapchain,
                                                              &n_swapchain_index) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        if (!OVR_SUCCESS(::ovr_GetTextureSwapChainBufferGL(m_session,
                                                           m_ui_swapchain,
                                                           n_swapchain_index,
                                                          &libovr_ui_swapchain_texture_id) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        pfn_gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER,
                                m_ui_swapchain_helper_fb_ids[0]);
        pfn_gl_bind_framebuffer(GL_READ_FRAMEBUFFER,
                                m_ui_swapchain_helper_fb_ids[1]);

        pfn_gl_framebuffer_texture2d(GL_DRAW_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     libovr_ui_swapchain_texture_id,
                                     0); /* level */
        pfn_gl_framebuffer_texture2d(GL_READ_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     UI_SWAPCHAIN_TEXTURE_ID,
                                     0); /* level */

        pfn_gl_blit_framebuffer(0,                                        /* srcX0 */
                                0,                                        /* srcY0 */
                                m_right_eye_fov_texture_resolution.at(0), /* srcX1 */
                                m_right_eye_fov_texture_resolution.at(1), /* srcY1 */
                                0,                                        /* dstX0 */
                                0,                                        /* dstY0 */
                                m_right_eye_fov_texture_resolution.at(0), /* dstX1 */
                                m_right_eye_fov_texture_resolution.at(1), /* dstY1 */
                                GL_COLOR_BUFFER_BIT,
                                GL_NEAREST);

        if (!OVR_SUCCESS(::ovr_CommitTextureSwapChain(m_session,
                                                      m_ui_swapchain) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    m_eye_texture_acquisition_state = AcquisitionState::NONE;
    result                          = true;
end:
    return result;
}

VRPlaybackUniquePtr PlaybackOVR::create(const float&    in_horizontal_fov_degrees,
                                        const float&    in_aspect_ratio,
                                        const Settings* in_settings_ptr)
{
    VRPlaybackUniquePtr result_ptr;

    result_ptr.reset(new PlaybackOVR(in_horizontal_fov_degrees,
                                     in_aspect_ratio,
                                     in_settings_ptr) );

    AI_ASSERT(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!dynamic_cast<PlaybackOVR*>(result_ptr.get() )->init())
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void PlaybackOVR::deinit_for_bound_gl_context()
{
    for (auto& current_eye_gl_props : {m_left_eye_gl_props, m_right_eye_gl_props})
    {
        if (current_eye_gl_props.color_texture_swapchain != nullptr)
        {
            ::ovr_DestroyTextureSwapChain(m_session,
                                          current_eye_gl_props.color_texture_swapchain);
        }
    }

    if (m_ui_swapchain != nullptr)
    {
        ::ovr_DestroyTextureSwapChain(m_session,
                                      m_ui_swapchain);
    }

    {
        static const GLuint swapchain_ui_texture_id = UI_SWAPCHAIN_TEXTURE_ID;

        auto pfn_gl_delete_framebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(OpenGL::g_cached_gl_delete_framebuffers);
        auto pfn_gl_delete_textures     = reinterpret_cast<PFNGLDELETETEXTURESPROC>    (OpenGL::g_cached_gl_delete_textures);

        pfn_gl_delete_framebuffers(sizeof(m_ui_swapchain_helper_fb_ids) / sizeof(m_ui_swapchain_helper_fb_ids[0]),
                                   m_ui_swapchain_helper_fb_ids);
        pfn_gl_delete_framebuffers(1, /* n */
                                  &swapchain_ui_texture_id);
    }
}

float PlaybackOVR::get_current_pitch_angle() const
{
    return m_pitch_angle;
}

float PlaybackOVR::get_current_yaw_angle() const
{
    return m_yaw_angle;
}

float PlaybackOVR::get_eye_offset_x(const bool& in_left_eye) const
{
    AI_ASSERT(m_sensor_sample_time != DBL_MAX);

    auto result = (in_left_eye) ? m_eye_poses[0].Position.x
                                : m_eye_poses[1].Position.x;

    return result;
}

std::array<uint32_t, 2> PlaybackOVR::get_eye_texture_resolution(const bool& in_left_eye) const
{
    return (in_left_eye) ? m_left_eye_fov_texture_resolution
                         : m_right_eye_fov_texture_resolution;
}

std::array<uint32_t, 2> PlaybackOVR::get_hmd_resolution() const
{
    return std::array<uint32_t, 2>{static_cast<uint32_t>(m_hmd_descriptor.Resolution.w),
                                   static_cast<uint32_t>(m_hmd_descriptor.Resolution.h)};
}

uint32_t PlaybackOVR::get_preview_texture_gl_id() const
{
    return m_mirror_texture_id;
}

float PlaybackOVR::get_tan_between_view_vec_and_bottom_fov_edge(const bool& in_left_eye) const
{
    return (in_left_eye) ? m_fov_ports[0].DownTan
                         : m_fov_ports[1].DownTan;
}

float PlaybackOVR::get_tan_between_view_vec_and_top_fov_edge(const bool& in_left_eye) const
{
    return (in_left_eye) ? m_fov_ports[0].UpTan
                         : m_fov_ports[1].UpTan;
}

bool PlaybackOVR::init()
{
    bool result = false;

    /* 1. Create ovr session instance. */
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
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    if (!OVR_SUCCESS(::ovr_Create(&m_session,
                                  &m_graphics_luid) ))
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    /* 2. Retrieve HMD descriptor. */
    m_hmd_descriptor = ::ovr_GetHmdDesc(m_session);

    /* 3. Set up FOV to use */
    m_fov_ports[0] = m_hmd_descriptor.DefaultEyeFov[0];
    m_fov_ports[1] = m_hmd_descriptor.DefaultEyeFov[1];

    /* 4. Cache eye texture resolutions. */
    {
        const auto left_fov_texture_size  = ::ovr_GetFovTextureSize(m_session,
                                                                    ::ovrEye_Left,
                                                                    m_fov_ports[0],
                                                                    1.0f); /* pixelsPerDisplayPixels */
        const auto right_fov_texture_size = ::ovr_GetFovTextureSize(m_session,
                                                                    ::ovrEye_Right,
                                                                    m_fov_ports[1],
                                                                    1.0f); /* pixelsPerDisplayPixels */

        m_left_eye_fov_texture_resolution  = std::array<uint32_t, 2>{static_cast<uint32_t>(left_fov_texture_size.w),
                                                                     static_cast<uint32_t>(left_fov_texture_size.h)};
        m_right_eye_fov_texture_resolution = std::array<uint32_t, 2>{static_cast<uint32_t>(right_fov_texture_size.w),
                                                                     static_cast<uint32_t>(right_fov_texture_size.h)};
    }

    /* 5. Misc init */
    {
        m_eye_render_descriptors[0] = ::ovr_GetRenderDesc(m_session,
                                                          ::ovrEye_Left,
                                                          m_fov_ports[0]);
        m_eye_render_descriptors[1] = ::ovr_GetRenderDesc(m_session,
                                                          ::ovrEye_Right,
                                                          m_fov_ports[1]);

        m_hmd_to_eye_pose[0] = m_eye_render_descriptors[0].HmdToEyePose;
        m_hmd_to_eye_pose[1] = m_eye_render_descriptors[1].HmdToEyePose;
    }

    result = true;
end:
    return result;
}

bool PlaybackOVR::present()
{
    ovrLayerEyeFov layer_3d = {};
    ovrLayerQuad   layer_ui = {};
    bool           result   = false;

    AI_ASSERT(m_sensor_sample_time != DBL_MAX);

    layer_ui.ColorTexture                 = m_ui_swapchain;
    layer_ui.Header.Flags                 = ovrLayerFlag_TextureOriginAtBottomLeft;
    layer_ui.Header.Type                  = ovrLayerType_Quad;
    layer_ui.QuadPoseCenter.Orientation.x = 0.0f;
    layer_ui.QuadPoseCenter.Orientation.y = 0.0f;
    layer_ui.QuadPoseCenter.Orientation.z = 0.0f;
    layer_ui.QuadPoseCenter.Orientation.w = 1.0f;
    layer_ui.QuadPoseCenter.Position.x    = 0.0f;
    layer_ui.QuadPoseCenter.Position.y    = 0.0f;
    layer_ui.QuadPoseCenter.Position.z    = -1.0f;
    layer_ui.QuadSize.x                   = 0.75f;
    layer_ui.QuadSize.y                   = 0.75f;
    layer_ui.Viewport.Pos.x               = 0;
    layer_ui.Viewport.Pos.y               = 0;
    layer_ui.Viewport.Size.h              = m_left_eye_fov_texture_resolution.at(1);
    layer_ui.Viewport.Size.w              = m_left_eye_fov_texture_resolution.at(0);

    layer_3d.ColorTexture[0]        = m_left_eye_gl_props.color_texture_swapchain;
    layer_3d.ColorTexture[1]        = m_right_eye_gl_props.color_texture_swapchain;
    layer_3d.Fov         [0]        = m_fov_ports[0];
    layer_3d.Fov         [1]        = m_fov_ports[1];
    layer_3d.Header.Flags           = ovrLayerFlag_TextureOriginAtBottomLeft;
    layer_3d.Header.Type            = ovrLayerType_EyeFov;
    layer_3d.RenderPose  [0]        = m_eye_poses[0];
    layer_3d.RenderPose  [1]        = m_eye_poses[1];
    layer_3d.SensorSampleTime       = m_sensor_sample_time;
    layer_3d.Viewport    [0].Pos.x  = 0;
    layer_3d.Viewport    [0].Pos.y  = 0;
    layer_3d.Viewport    [0].Size.w = m_left_eye_fov_texture_resolution.at(0);
    layer_3d.Viewport    [0].Size.h = m_left_eye_fov_texture_resolution.at(1);
    layer_3d.Viewport    [1].Pos.x  = 0;
    layer_3d.Viewport    [1].Pos.y  = 0;
    layer_3d.Viewport    [1].Size.w = m_right_eye_fov_texture_resolution.at(0);
    layer_3d.Viewport    [1].Size.h = m_right_eye_fov_texture_resolution.at(1);

    /* Make sure to use HFOV settings as specified by the class user. */
    {
        const float hfov_radians  = m_horizontal_fov_degrees / 360.0f * 2.0f * 3.1416f;
        const float tan_hfov_half = tan(hfov_radians / 2);
        const float tan_vfov_half = tan_hfov_half * (static_cast<float>(m_left_eye_fov_texture_resolution.at(0) ) / static_cast<float>(m_left_eye_fov_texture_resolution.at(1) ));

        layer_3d.Fov[0].LeftTan  = tan_hfov_half;
        layer_3d.Fov[0].RightTan = tan_hfov_half;
        layer_3d.Fov[0].UpTan    = tan_vfov_half;
        layer_3d.Fov[0].DownTan  = tan_vfov_half;
        layer_3d.Fov[1]          = layer_3d.Fov[0];
    }

    {
        ::ovrLayerHeader* layers[2] =
        {
            &layer_3d.Header,
            &layer_ui.Header
        };

        if (!OVR_SUCCESS(::ovr_SubmitFrame(m_session,
                                           m_n_frames_presented,
                                           nullptr, /* viewScaleDesc */
                                           layers,
                                           sizeof(layers) / sizeof(layers[0]) )))
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    m_n_frames_presented ++;
    result               =  true;
end:
    return result;
}

bool PlaybackOVR::setup_for_bound_gl_context(const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2,
                                             HDC                            /* in_window_dc */,
                                             HGLRC                          /* in_glrc      */)
{
    bool result = false;

    /* 1. Set up texture swapchains for each eye. */
    {
        ovrTextureSwapChainDesc swapchain_desc = {};

        swapchain_desc.ArraySize   = 1;
        swapchain_desc.BindFlags   = 0;
        swapchain_desc.Format      = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        swapchain_desc.Height      = m_left_eye_fov_texture_resolution.at(1);
        swapchain_desc.MipLevels   = 1;
        swapchain_desc.MiscFlags   = 0;
        swapchain_desc.SampleCount = 1;
        swapchain_desc.StaticImage = ovrFalse;
        swapchain_desc.Type        = ovrTexture_2D;
        swapchain_desc.Width       = m_left_eye_fov_texture_resolution.at(0);

        if (!OVR_SUCCESS(::ovr_CreateTextureSwapChainGL(m_session,
                                                       &swapchain_desc,
                                                       &m_left_eye_gl_props.color_texture_swapchain) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        if (!OVR_SUCCESS(::ovr_CreateTextureSwapChainGL(m_session,
                                                       &swapchain_desc,
                                                       &m_ui_swapchain) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        swapchain_desc.Height = m_right_eye_fov_texture_resolution.at(1);
        swapchain_desc.Width  = m_right_eye_fov_texture_resolution.at(0);

        if (!OVR_SUCCESS(::ovr_CreateTextureSwapChainGL(m_session,
                                                       &swapchain_desc,
                                                       &m_right_eye_gl_props.color_texture_swapchain) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    for (auto& current_texture_swapchain : {m_left_eye_gl_props.color_texture_swapchain,
                                            m_right_eye_gl_props.color_texture_swapchain,
                                            m_ui_swapchain})
    {
        int n_swapchain_textures = 0;

        if (!OVR_SUCCESS(::ovr_GetTextureSwapChainLength(m_session,
                                                         current_texture_swapchain,
                                                        &n_swapchain_textures) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        for (int n_swapchain_texture = 0;
                 n_swapchain_texture < n_swapchain_textures;
               ++n_swapchain_texture)
        {
            uint32_t current_texture_gl_id = 0;

            ::ovr_GetTextureSwapChainBufferGL(m_session,
                                              current_texture_swapchain,
                                              n_swapchain_texture,
                                             &current_texture_gl_id);

            glBindTexture(GL_TEXTURE_2D,
                          current_texture_gl_id);

            glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
        }
    }

    /* 2. Create a mirror texture we're going to use to show game's visuals on monitor (after processing) */
    {
        ::ovrMirrorTextureDesc mirror_texture_descriptor{};

        mirror_texture_descriptor.Format        = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        mirror_texture_descriptor.Height        = in_preview_texture_extents_u32vec2.at(1);
        mirror_texture_descriptor.MirrorOptions = ovrMirrorOption_Default;
        mirror_texture_descriptor.MiscFlags     = 0;
        mirror_texture_descriptor.Width         = in_preview_texture_extents_u32vec2.at(0);

        if (!OVR_SUCCESS(::ovr_CreateMirrorTextureGL(m_session,
                                                    &mirror_texture_descriptor,
                                                    &m_mirror_texture) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        if (!OVR_SUCCESS(::ovr_GetMirrorTextureBufferGL(m_session,
                                                        m_mirror_texture,
                                                       &m_mirror_texture_id) ))
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        m_preview_texture_extents_u32vec2 = in_preview_texture_extents_u32vec2;
    }

    /* 3. Libovr does not correctly handle situations where we have more than one swapchain acquired at the same time.
     *    In order to be able to render both left eye's 3D view AND UI, we allocate an extra texture we will be rendering to.
     *    Once both viewpoints are rendered, we're going to acquire the UI swapchain, blit contents from the extra texture to the
     *    said swapchain, and commit it.
     */
    {
        auto pfn_gl_bind_texture     = reinterpret_cast<PFNGLBINDTEXTUREPROC>    (OpenGL::g_cached_gl_bind_texture);
        auto pfn_gl_gen_framebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(OpenGL::g_cached_gl_gen_framebuffers);
        auto pfn_gl_tex_image2d      = reinterpret_cast<PFNGLTEXIMAGE2DPROC>     (OpenGL::g_cached_gl_tex_image_2D);
        auto pfn_gl_tex_parameteri   = reinterpret_cast<PFNGLTEXPARAMETERIPROC>  (OpenGL::g_cached_gl_tex_parameteri);

        pfn_gl_bind_texture(GL_TEXTURE_2D,
                            UI_SWAPCHAIN_TEXTURE_ID);
        pfn_gl_tex_image2d (GL_TEXTURE_2D,
                            0,                                      /* level          */
                            GL_RGBA8,                               /* internalformat */
                            m_left_eye_fov_texture_resolution.at(0),
                            m_left_eye_fov_texture_resolution.at(1),
                            0,                                      /* border         */
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            nullptr);

        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_MIN_FILTER,
                              GL_LINEAR);
        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_MAG_FILTER,
                              GL_LINEAR);
        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE);
        pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                              GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE);

        pfn_gl_gen_framebuffers(sizeof(m_ui_swapchain_helper_fb_ids) / sizeof(m_ui_swapchain_helper_fb_ids[0]),
                                m_ui_swapchain_helper_fb_ids);
    }

    result = true;
end:
    return result;
}