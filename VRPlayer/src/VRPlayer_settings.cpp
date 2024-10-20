/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_file_serializer.h"
#include "VRPlayer_settings.h"

static const char*                                g_settings_filename_ptr       = "q1_vr_player_settings.txt";
static const std::map<std::string, Variant::Type> g_setting_to_variant_type_map =
{
    {"ConsoleWindowYOffset",      Variant::Type::I32},
    {"EyeSeparationMultiplier",   Variant::Type::FP32},
    {"OrthoSeparationMultiplier", Variant::Type::FP32},
    {"StatusBarYOffset",          Variant::Type::I32},
    {"UIScale",                   Variant::Type::FP32}
};

Settings::Settings()
    :m_console_window_y_offset     (630),
     m_eye_separation_multiplier   (0.5f),
     m_ortho_separation_multiplier (0.05f),
     m_status_bar_y_offset         (630),
     m_ui_scale                    (0.374f),
     m_viewport_offset_x_multiplier(0.1f),
     m_viewport_offset_y_multiplier(0.1f)
{
    /* Stub */
}

Settings::~Settings()
{
    /* Store settings for re-use in the future. */
    auto serializer_ptr = FileSerializer::create_for_writing(g_settings_filename_ptr,
                                                             g_setting_to_variant_type_map);

    if (serializer_ptr != nullptr)
    {
        serializer_ptr->set_i32 ("ConsoleWindowYOffset",
                                 m_console_window_y_offset);
        serializer_ptr->set_fp32("EyeSeparationMultiplier",
                                 m_eye_separation_multiplier);
        serializer_ptr->set_fp32("OrthoSeparationMultiplier",
                                 m_ortho_separation_multiplier);
        serializer_ptr->set_i32 ("StatusBarYOffset",
                                 m_console_window_y_offset);
        serializer_ptr->set_fp32("UIScale",
                                 m_ui_scale);
    }
}

SettingsUniquePtr Settings::create()
{
    SettingsUniquePtr result_ptr;

    result_ptr.reset(new Settings() );

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

bool Settings::init()
{
    bool result         = false;
    auto serializer_ptr = FileSerializer::create_for_reading(g_settings_filename_ptr,
                                                             g_setting_to_variant_type_map);

    /* Try to load settings from the most recent session. */
    if (serializer_ptr != nullptr)
    {
        serializer_ptr->get_i32 ("ConsoleWindowYOffset",
                                &m_console_window_y_offset);
        serializer_ptr->get_fp32("EyeSeparationMultiplier",
                                &m_eye_separation_multiplier);
        serializer_ptr->get_fp32("OrthoSeparationMultiplier",
                                &m_ortho_separation_multiplier);
        serializer_ptr->get_i32("StatusBarYOffset",
                                &m_console_window_y_offset);
        serializer_ptr->get_fp32("UIScale",
                                &m_ui_scale);
    }

    result = true;
    return result;
}