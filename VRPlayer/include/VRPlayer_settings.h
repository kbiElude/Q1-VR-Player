/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_SETTINGS_H)
#define VRPLAYER_SETTINGS_H

#include "VRPlayer_types.h"

class Settings
{
public:
    /* Public functions */
    static SettingsUniquePtr create();

    const float& get_eye_separation_multiplier() const
    {
        return m_eye_separation_multiplier;
    }

    float* get_eye_separation_multiplier_ptr()
    {
        return &m_eye_separation_multiplier;
    }

    const float& get_ui_quad_distance() const
    {
        return m_ui_quad_distance;
    }

    float* get_ui_quad_distance_ptr()
    {
        return &m_ui_quad_distance;
    }

    const float& get_viewport_offset_x_multiplier() const
    {
        return m_viewport_offset_x_multiplier;
    }

    const float& get_viewport_offset_y_multiplier() const
    {
        return m_viewport_offset_y_multiplier;
    }

    float* get_viewport_offset_x_multiplier_ptr()
    {
        return &m_viewport_offset_x_multiplier;
    }

    float* get_viewport_offset_y_multiplier_ptr()
    {
        return &m_viewport_offset_y_multiplier;
    }

    ~Settings();

private:
    /* Private funcs */
    Settings();

    bool init();

    /* Private vars */
    float m_eye_separation_multiplier;

    float m_ui_quad_distance;
    float m_viewport_offset_x_multiplier;
    float m_viewport_offset_y_multiplier;
};

#endif /* VRPLAYER_SETTINGS_H */