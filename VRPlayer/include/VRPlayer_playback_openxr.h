/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_PLAYBACK_OPENXR_H)
#define VRPLAYER_PLAYBACK_OPENXR_H

#include "VRPlayer_types.h"

/* OpenXR headers need a few extra nudges to include the right stuff.. */
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32

struct IUnknown;

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"


class PlaybackOpenXR : public IVRPlayback
{
public:
    /* Public funcs */

    static VRPlaybackUniquePtr create(const float&    in_horizontal_fov_degrees,
                                      const float&    in_aspect_ratio,
                                      const Settings* in_settings_ptr);


    bool                    acquire_eye_texture                         (const bool&                    in_left_eye,
                                                                         uint32_t*                      out_eye_color_texture_id_ptr,
                                                                         uint32_t*                      out_eye_color_texture_n_layer_ptr,
                                                                         uint32_t*                      out_ui_color_texture_id_ptr,
                                                                         uint32_t*                      out_ui_color_texture_n_layer_ptr)         final;
    bool                    commit_eye_texture                          ()                                                                        final;
    float                   get_current_pitch_angle                     ()                                                                  const final; // rotation along Y
    float                   get_current_yaw_angle                       ()                                                                  const final; // rotation along X
    float                   get_eye_offset_x                            (const bool&                    in_left_eye)                        const final;
    std::array<uint32_t, 2> get_eye_texture_resolution                  (const bool&                    in_left_eye)                        const final;
    uint32_t                get_preview_texture_gl_id                   ()                                                                  const final;
    float                   get_tan_between_view_vec_and_bottom_fov_edge(const bool&                    in_left_eye)                        const final;
    float                   get_tan_between_view_vec_and_top_fov_edge   (const bool&                    in_left_eye)                        const final;
    bool                    present                                     ()                                                                        final;

    void                    deinit_for_bound_gl_context                 ()                                                                        final;
    bool                    setup_for_bound_gl_context                  (const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2,
                                                                         HDC                            in_window_dc,
                                                                         HGLRC                          in_glrc)                                  final;

    ~PlaybackOpenXR();

    bool needs_manual_viewport_adjustment() const final
    {
        return true;
    }

    bool supports_separate_ui_texture() const final
    {
        return true;
    }

private:
    /* Private type decls */
    struct PerEyeProps
    {
        std::array<uint32_t, 2> eye_texture_extents         = {};
        XrFovf                  fov                         = {};
        bool                    is_active                   = false;
        uint32_t                n_acquired_swapchain_image  = UINT32_MAX;
        XrPosef                 pose                        = {};
        std::vector<uint32_t>   swapchain_texture_gl_id_vec = {};
        uint64_t                xr_swapchain                = 0;
    };

    /* Private funcs */
    PlaybackOpenXR(const float&    in_horizontal_fov_degrees,
                   const float&    in_aspect_ratio,
                   const Settings* in_settings_ptr);

    bool init();

    /* Private vars */
    const float           m_aspect_ratio;
    const float           m_horizontal_fov_degrees;
    const Settings* const m_settings_ptr;

    PFN_xrGetOpenGLGraphicsRequirementsKHR m_pfn_xr_get_opengl_gfx_requirements_khr;

    uint64_t       m_xr_instance;
    uint64_t       m_xr_session;
    XrSessionState m_xr_session_state;
    uint64_t       m_xr_space;
    uint64_t       m_xr_system_id;

    uint32_t m_gl_blit_src_texture_fb;
    uint32_t m_gl_preview_texture_fb;
    uint32_t m_gl_preview_texture_id;

    XrTime      m_current_frame_display_time;
    bool        m_current_frame_should_render;
    PerEyeProps m_eye_props                 [2];
    float       m_pitch_angle;
    float       m_yaw_angle;
};

#endif /* VRPLAYER_PLAYBACK_OPENXR_H */