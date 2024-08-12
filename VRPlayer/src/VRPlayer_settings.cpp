/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "VRPlayer_settings.h"

Settings::Settings()
    :m_console_window_y_offset  (480),
     m_eye_separation_multiplier(4.5f),
     m_status_bar_y_offset      (282),
     m_ui_scale                 (0.541f)
{
    /* Stub */
}

Settings::~Settings()
{
    /* Stub */
}

SettingsUniquePtr Settings::create()
{
    SettingsUniquePtr result_ptr;

    result_ptr.reset(new Settings() );

    AI_ASSERT(result_ptr != nullptr);
    return result_ptr;
}