/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_defines.inl"
#include "OpenGL/globals.h"
#include "VRPlayer_playback_openxr.h"
#include "VRPlayer_settings.h"

PlaybackOpenXR::PlaybackOpenXR(const float&    in_horizontal_fov_degrees,
                               const float&    in_aspect_ratio,
                               const Settings* in_settings_ptr)
    :m_aspect_ratio          (in_aspect_ratio),
     m_horizontal_fov_degrees(in_horizontal_fov_degrees),
     m_settings_ptr          (in_settings_ptr)
{
    /* Stub */
}

PlaybackOpenXR::~PlaybackOpenXR()
{
    /* Stub */
}

bool PlaybackOpenXR::acquire_eye_texture(const bool& in_left_eye,
                                         uint32_t*   out_eye_color_texture_id_ptr)
{
    // todo
    assert(false);

    in_left_eye;
    out_eye_color_texture_id_ptr;

    return false;
}

bool PlaybackOpenXR::commit_eye_texture()
{
    // todo
    assert(false);

    return false;
}

PlaybackOpenXRUniquePtr PlaybackOpenXR::create(const float&    in_horizontal_fov_degrees,
                                               const float&    in_aspect_ratio,
                                               const Settings* in_settings_ptr)
{
    PlaybackOpenXRUniquePtr result_ptr;

    result_ptr.reset(new PlaybackOpenXR(in_horizontal_fov_degrees,
                                        in_aspect_ratio,
                                        in_settings_ptr) );

    AI_ASSERT(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void PlaybackOpenXR::deinit_for_bound_gl_context()
{
    // todo
    assert(false);
}

float PlaybackOpenXR::get_current_pitch_angle() const
{
    // todo
    assert(false);

    return 0.0f;
}

float PlaybackOpenXR::get_current_yaw_angle() const
{
    // todo
    assert(false);

    return 0;
}

float PlaybackOpenXR::get_eye_offset_x(const bool& in_left_eye,
                                       const bool& in_multiply_by_user_setting) const
{
    // todo
    assert(false);

    in_left_eye;
    in_multiply_by_user_setting;

    return 0;
}

std::array<uint32_t, 2> PlaybackOpenXR::get_eye_texture_resolution(const bool& in_left_eye) const
{
    // todo
    assert(false);

    in_left_eye;

    return {};
}

uint32_t PlaybackOpenXR::get_preview_texture_gl_id() const
{
    // todo
    assert(false);

    return 0;
}

float PlaybackOpenXR::get_tan_between_view_vec_and_bottom_fov_edge(const bool& in_left_eye) const
{
    // todo
    assert(false);

    in_left_eye;

    return 0;
}

float PlaybackOpenXR::get_tan_between_view_vec_and_top_fov_edge(const bool& in_left_eye) const
{
    // todo
    assert(false);

    in_left_eye;

    return 0;
}

bool PlaybackOpenXR::init()
{
    // todo
    assert(false);

    return false;
}

bool PlaybackOpenXR::present()
{
    // todo
    assert(false);

    return false;
}

bool PlaybackOpenXR::setup_for_bound_gl_context(const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2)
{
    // todo
    assert(false);

    in_preview_texture_extents_u32vec2;

    return false;
}