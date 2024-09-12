/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_PLAYBACK_OVR_H)
#define VRPLAYER_PLAYBACK_OVR_H

#include "VRPlayer_types.h"
#include "OVR_CAPI_GL.h"

class PlaybackOVR : public IVRPlayback
{
public:
    /* Public funcs */

    static PlaybackOVRUniquePtr create(const float&    in_horizontal_fov_degrees,
                                       const float&    in_aspect_ratio,
                                       const Settings* in_settings_ptr);


    std::array<uint32_t, 2> get_hmd_resolution() const;

    bool                    acquire_eye_texture                         (const bool&                    in_left_eye,
                                                                         uint32_t*                      out_eye_color_texture_id_ptr)             final;
    bool                    commit_eye_texture                          ()                                                                        final;
    float                   get_current_pitch_angle                     ()                                                                  const final; // rotation along Y
    float                   get_current_yaw_angle                       ()                                                                  const final; // rotation along X
    float                   get_eye_offset_x                            (const bool&                    in_left_eye,
                                                                         const bool&                    in_multiply_by_user_setting)        const final;
    std::array<uint32_t, 2> get_eye_texture_resolution                  (const bool&                    in_left_eye)                        const final;
    uint32_t                get_preview_texture_gl_id                   ()                                                                  const final;
    float                   get_tan_between_view_vec_and_bottom_fov_edge(const bool&                    in_left_eye)                        const final;
    float                   get_tan_between_view_vec_and_top_fov_edge   (const bool&                    in_left_eye)                        const final;
    bool                    present                                     ()                                                                        final;

    void                    deinit_for_bound_gl_context                 ()                                                                        final;
    bool                    setup_for_bound_gl_context                  (const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2)       final;

    ~PlaybackOVR();

private:
    /* Private type decls */
    enum class AcquisitionState : uint8_t
    {
        NONE,
        LEFT_EYE,
        RIGHT_EYE,

        UNKNOWN
    };

    typedef struct EyeGLProps
    {
        ::ovrTextureSwapChain color_texture_swapchain = nullptr;
    } EyeGLProps;

    /* Private funcs */
    PlaybackOVR(const float&    in_horizontal_fov_degrees,
                const float&    in_aspect_ratio,
                const Settings* in_settings_ptr);

    bool init();

    /* Private vars */
    const float             m_aspect_ratio;
    const float             m_horizontal_fov_degrees;
    std::array<uint32_t, 2> m_preview_texture_extents_u32vec2;
    const Settings* const   m_settings_ptr;

    ovrGraphicsLuid m_graphics_luid;
    ovrHmdDesc      m_hmd_descriptor;
    ovrSession      m_session;

    std::array<uint32_t, 2> m_left_eye_fov_texture_resolution;
    std::array<uint32_t, 2> m_right_eye_fov_texture_resolution;

    EyeGLProps m_left_eye_gl_props;
    EyeGLProps m_right_eye_gl_props;

    ovrPosef         m_eye_poses             [2];
    ovrEyeRenderDesc m_eye_render_descriptors[2];
    ovrFovPort       m_fov_ports             [2];
    ovrPosef         m_hmd_to_eye_pose       [2];
    ovrMirrorTexture m_mirror_texture;
    uint32_t         m_mirror_texture_id;
    double           m_sensor_sample_time;

    float            m_pitch_angle;
    float            m_yaw_angle;

    AcquisitionState m_eye_texture_acquisition_state;
    uint64_t         m_n_frames_presented;
};

#endif /* VRPLAYER_PLAYBACK_OVR_H */