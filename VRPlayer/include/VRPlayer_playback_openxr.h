/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_PLAYBACK_OPENXR_H)
#define VRPLAYER_PLAYBACK_OPENXR_H

#include "VRPlayer_types.h"

class PlaybackOpenXR : public IVRPlayback
{
public:
    /* Public funcs */

    static PlaybackOpenXRUniquePtr create(const float&    in_horizontal_fov_degrees,
                                          const float&    in_aspect_ratio,
                                          const Settings* in_settings_ptr);


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

    ~PlaybackOpenXR();

private:
    /* Private type decls */

    /* Private funcs */
    PlaybackOpenXR(const float&    in_horizontal_fov_degrees,
                   const float&    in_aspect_ratio,
                   const Settings* in_settings_ptr);

    bool init();

    /* Private vars */
    const float           m_aspect_ratio;
    const float           m_horizontal_fov_degrees;
    const Settings* const m_settings_ptr;
};

#endif /* VRPLAYER_PLAYBACK_OPENXR_H */