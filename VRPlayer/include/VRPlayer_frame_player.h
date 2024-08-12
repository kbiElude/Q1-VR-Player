/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_FRAME_PLAYER_H)
#define VR_PLAYER_FRAME_PLAYER_H

#include "VRPlayer_types.h"

class FramePlayer
{
public:
    /* Public funcs */

    static FramePlayerUniquePtr create(const IVRPlayback* in_vr_playback_ptr,
                                       const Settings*    in_settings_ptr);

    void play(const Frame* in_frame_ptr,
              const bool&  in_left_eye);

    ~FramePlayer();

private:
    /* Private funcs */
    FramePlayer(const IVRPlayback* in_vr_playback_ptr,
                const Settings*    in_settings_ptr);


    /* Private vars */

    std::unordered_map<uint32_t, uint32_t> m_game_to_this_context_texture_gl_id_map;

    const Settings*    const m_settings_ptr;
    const IVRPlayback* const m_vr_playback_ptr;
};

#endif /* VR_PLAYER_FRAME_PLAYER_H */