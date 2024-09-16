/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_misc.h"
#include "Launcher_state.h"
#include <assert.h>
#include "common_file_serializer.h"

static const char* g_active_vr_backend_key_ptr = "ActiveVRBackend";
static const char* g_gl_quake_exe_path_key_ptr = "GLQuakeExePath";

static const std::map<std::string, Variant::Type> g_serializer_settings =
{
    {g_active_vr_backend_key_ptr, Variant::Type::U8_TEXT_STRING},
    {g_gl_quake_exe_path_key_ptr, Variant::Type::U8_TEXT_STRING}
};


Launcher::State::State()
    :m_active_vr_backend(VRBackend::UNKNOWN)
{
    /* Stub */
}

Launcher::State::~State()
{
    /* Store the specified locaion for future reuse. */
    auto serializer_ptr = FileSerializer::create_for_writing("q1_vr_launcher_settings.txt",
                                                             g_serializer_settings);

    if (serializer_ptr != nullptr)
    {
        const uint8_t* active_vr_backend_ptr = Misc::get_u8_text_string_for_vr_backend(m_active_vr_backend);
        std::string    path_u8               = std::string                            (m_glquake_exe_file_path.begin(),
                                                                                       m_glquake_exe_file_path.end  () );

        serializer_ptr->set_u8_text_string(g_active_vr_backend_key_ptr,
                                           active_vr_backend_ptr,
                                           strlen(reinterpret_cast<const char*>(active_vr_backend_ptr) ));
        serializer_ptr->set_u8_text_string(g_gl_quake_exe_path_key_ptr,
                                           reinterpret_cast<const uint8_t*>    (path_u8.c_str() ),
                                           path_u8.size                        () );
    }
}

Launcher::StateUniquePtr Launcher::State::create()
{
    Launcher::StateUniquePtr result_ptr;

    result_ptr.reset(new State() );

    assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Launcher::State::init()
{
    bool result = false;

    /* Try to initialize state using the settings file first.. */
    {
        auto serializer_ptr = FileSerializer::create_for_reading("q1_vr_launcher_settings.txt",
                                                                 g_serializer_settings);

        if (serializer_ptr != nullptr)
        {
            const uint8_t* active_vr_backend_u8_ptr     = nullptr;
            uint32_t       active_vr_backend_u8_n_bytes = 0;
            uint32_t       file_path_u8_n_bytes         = 0;
            const uint8_t* file_path_u8_ptr             = nullptr;

            if (serializer_ptr->get_u8_text_string(g_active_vr_backend_key_ptr,
                                                  &active_vr_backend_u8_ptr,
                                                  &active_vr_backend_u8_n_bytes) )
            {
                auto active_vr_backend = std::string(reinterpret_cast<const char*>(active_vr_backend_u8_ptr),
                                                     active_vr_backend_u8_n_bytes);

                m_active_vr_backend = Misc::get_vr_backend_for_text_string(active_vr_backend);
            }

            if (serializer_ptr->get_u8_text_string(g_gl_quake_exe_path_key_ptr,
                                                  &file_path_u8_ptr,
                                                  &file_path_u8_n_bytes) )
            {
                /* TODO: This is not going to work at all for paths with wide characters. */
                auto string_u8 = std::string(reinterpret_cast<const char*>(file_path_u8_ptr),
                                             file_path_u8_n_bytes);

                m_glquake_exe_file_path = std::wstring(string_u8.begin(),
                                                       string_u8.end  () );
            }
        }
    }

    result = true;
    return result;
}