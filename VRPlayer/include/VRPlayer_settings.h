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

    const int32_t& get_console_window_y_offset() const
    {
        return m_console_window_y_offset;
    }

    int32_t* get_console_window_y_offset_ptr()
    {
        return &m_console_window_y_offset;
    }

    const float& get_eye_separation_multiplier() const
    {
        return m_eye_separation_multiplier;
    }

    float* get_eye_separation_multiplier_ptr()
    {
        return &m_eye_separation_multiplier;
    }

    const float& get_ortho_separation_multiplier() const
    {
        return m_ortho_separation_multiplier;
    }

    float* get_ortho_separation_multiplier_ptr()
    {
        return &m_ortho_separation_multiplier;
    }

    const int32_t& get_status_bar_y_offset() const
    {
        return m_status_bar_y_offset;
    }

    int32_t* get_status_bar_y_offset_ptr()
    {
        return &m_status_bar_y_offset;
    }

    const float& get_ui_scale() const
    {
        return m_ui_scale;
    }

    float* get_ui_scale_ptr()
    {
        return &m_ui_scale;
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
    int32_t m_console_window_y_offset;
    float   m_eye_separation_multiplier;
    float   m_ortho_separation_multiplier;
    int32_t m_status_bar_y_offset;
    float   m_ui_scale;

    float m_viewport_offset_x_multiplier;
    float m_viewport_offset_y_multiplier;
};

#endif /* VRPLAYER_SETTINGS_H */