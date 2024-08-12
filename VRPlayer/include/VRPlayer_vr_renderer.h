/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_VR_RENDERER_H)
#define VR_PLAYER_VR_RENDERER_H

#include "VRPlayer_types.h"

class VRRenderer
{
public:
    /* Public funcs */

    static VRRendererUniquePtr create(FramePlayer* in_frame_player_ptr,
                                      IVRPlayback* in_vr_playback_ptr);

    bool render                 (const Frame* in_frame_ptr);
    bool setup_for_bound_context();

    ~VRRenderer();

private:
    /* Private funcs */
    VRRenderer(FramePlayer* in_frame_player_ptr,
               IVRPlayback* in_vr_playback_ptr);

    void render_eye_frames(const bool&     in_left_eye,
                           const uint32_t& in_eye_texture_gl_id,
                           const Frame*    in_frame_ptr);

    /* Private vars */
    FramePlayer* m_frame_player_ptr;
    IVRPlayback* m_vr_playback_ptr;

    uint32_t m_eye0_fb_id;
    uint32_t m_eye1_fb_id;

    uint32_t m_eye0_depth_texture_id;
    uint32_t m_eye1_depth_texture_id;

    std::unordered_map<uint32_t, uint32_t> m_game_to_this_context_texture_gl_id_map;
};

#endif /* VR_PLAYER_VR_RENDERER_H */