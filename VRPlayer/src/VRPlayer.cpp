/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "APIInterceptor/include/Common/callbacks.h"
#include "common_defines.inl"
#include "common_misc.h"
#include "VRPlayer.h"
#include "VRPlayer_frame_interceptor.h"
#include "VRPlayer_frame_player.h"
#include "VRPlayer_playback_openxr.h"
#include "VRPlayer_playback_ovr.h"
#include "VRPlayer_preview_window.h"
#include "VRPlayer_settings.h"
#include "VRPlayer_slab_allocator.h"
#include "VRPlayer_vr_renderer.h"
#include <cassert>

VRPlayer* g_vr_player_ptr = nullptr;


#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif


VRPlayer::VRPlayer()
    :m_q1_hwnd(0)
{
    /* Stub */
}

VRPlayer::~VRPlayer()
{
    // NOTE: The order used below matters.
    m_preview_window_ptr.reset   ();
    m_vr_renderer_ptr.reset      ();
    m_frame_interceptor_ptr.reset();
    m_frame_player_ptr.reset     ();
    m_vr_playback_ptr.reset      ();
    m_settings_ptr.reset         ();
    m_slab_allocator_ptr.reset   ();

    g_vr_player_ptr = nullptr;
}

VRPlayerUniquePtr VRPlayer::create()
{
    VRPlayerUniquePtr result_ptr(new VRPlayer() );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
        else
        {
            assert(g_vr_player_ptr == nullptr);

            g_vr_player_ptr = result_ptr.get();
        }
    }

    return result_ptr;
}

bool VRPlayer::init()
{
    bool result = false;

    // NOTE: The order in which we initialize objects below is important.
    m_slab_allocator_ptr = SlabAllocator::create();
    m_settings_ptr       = Settings::create     ();

    {
        /* Determine which VR backend to use .. */
        auto buffer_u8_vec       = std::vector<uint8_t>(32767); // max size as per MSDN docs.
        auto selected_vr_backend = Common::VRBackend::UNKNOWN;

        if (::GetEnvironmentVariable("VR_BACKEND",
                                     reinterpret_cast<char*>(buffer_u8_vec.data() ),
                                     static_cast<DWORD>     (buffer_u8_vec.size() )) == 0)
        {
            ::MessageBox(HWND_DESKTOP,
                        "Could not retrieve VR_BACKEND environment variable data.",
                        "Error",
                        MB_ICONERROR | MB_OK);

            goto end;
        }

        selected_vr_backend = Common::Misc::get_vr_backend_for_text_string(reinterpret_cast<const char*>(buffer_u8_vec.data() ));

        // TODO: Simply use m_playback_ptr;
        switch (selected_vr_backend)
        {
            case Common::VRBackend::LIBOVR:
            {
                m_vr_playback_ptr = PlaybackOVR::create(90.0f,                       /* in_horizontal_fov_degrees */
                                                        1280.0f / 720.0f,            /* in_aspect_ratio           */
                                                        m_settings_ptr.get() );

                break;
            }

            case Common::VRBackend::OPENXR:
            {
                m_vr_playback_ptr = PlaybackOpenXR::create(90.0f,                       /* in_horizontal_fov_degrees */
                                                           1280.0f / 720.0f,            /* in_aspect_ratio           */
                                                           m_settings_ptr.get() );

                break;
            }

            default:
            {
                ::MessageBox(HWND_DESKTOP,
                             "Unrecognized VR backend requested by Launcher.",
                             "Error",
                             MB_ICONERROR | MB_OK);

                goto end;
            }
        }
    }

    m_frame_player_ptr      = FramePlayer::create     (m_vr_playback_ptr.get   (),
                                                       m_settings_ptr.get      () );
    m_vr_renderer_ptr       = VRRenderer::create      (m_frame_player_ptr.get  (),
                                                       m_vr_playback_ptr.get   () );
    m_preview_window_ptr    = PreviewWindow::create   (this,
                                                       m_vr_playback_ptr.get   (),
                                                       m_vr_renderer_ptr.get   (),
                                                       m_settings_ptr.get      () );
    m_frame_interceptor_ptr = FrameInterceptor::create(m_slab_allocator_ptr.get(),
                                                       m_preview_window_ptr.get() );

    /* Register for callbacks */
    APIInterceptor::register_for_callback(APIInterceptor::APIFUNCTION_WGL_WGLDELETECONTEXT,
                                         &on_q1_wgldeletecontext,
                                          this);
    APIInterceptor::register_for_callback(APIInterceptor::APIFUNCTION_WGL_WGLMAKECURRENT,
                                         &on_q1_wglmakecurrent,
                                          this);

    result = true;
end:
    return result;
}

void VRPlayer::on_q1_wgldeletecontext(APIInterceptor::APIFunction                in_api_func,
                                      uint32_t                                   in_n_args,
                                      const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                      void*                                      in_user_arg_ptr,
                                      bool*                                      out_should_pass_through_ptr)
{
    *out_should_pass_through_ptr = true;

    if (g_vr_player_ptr != nullptr)
    {
        delete g_vr_player_ptr;
        g_vr_player_ptr = nullptr;
    }
}

void VRPlayer::on_q1_wglmakecurrent(APIInterceptor::APIFunction                in_api_func,
                                    uint32_t                                   in_n_args,
                                    const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                    void*                                      in_user_arg_ptr,
                                    bool*                                      out_should_pass_through_ptr)
{
    auto      arg0_ptr = in_args_ptr[0].get_ptr();
    auto      arg1_ptr = in_args_ptr[1].get_ptr();
    VRPlayer* this_ptr = reinterpret_cast<VRPlayer*>(in_user_arg_ptr);

    if (arg1_ptr != nullptr)
    {
        HDC  q1_hdc  = reinterpret_cast<HDC>(reinterpret_cast<intptr_t>(arg0_ptr) );
        HWND q1_hwnd = ::WindowFromDC       (q1_hdc);

        if (q1_hwnd != nullptr)
        {
            if (this_ptr->m_q1_hwnd == nullptr)
            {
                this_ptr->m_q1_hwnd = q1_hwnd;

                /* Now that we know which window the game uses, reposition the windows accordingly */
                this_ptr->reposition_windows();
            }
            else
            {
                /* Wait this should never happen */
                assert(false);
            }
        }
    }

    *out_should_pass_through_ptr = true;
}

void VRPlayer::reposition_windows()
{
    // TODO: FIXME! Obtain window extents from environment variables set by the launcher.
    static const uint32_t Q1_WINDOW_WIDTH  = 2064;
    static const uint32_t Q1_WINDOW_HEIGHT = 2272;

    RECT q1_window_rect = {};

    ::GetWindowRect(m_q1_hwnd,
                   &q1_window_rect);

    /* Hide the Q1 window. We're going to present the preview window instead.  */
    ::ShowWindow(m_q1_hwnd,
                 SW_HIDE);

    /* Center our preview window */
    const auto desktop_width           = ::GetSystemMetrics(SM_CXSCREEN);
    const auto desktop_height          = ::GetSystemMetrics(SM_CYSCREEN);
    const auto eye_texture_height      = Q1_WINDOW_HEIGHT;
    const auto eye_texture_total_width = Q1_WINDOW_WIDTH * 2;

    const auto preview_extents     = std::array<uint32_t, 2>{eye_texture_total_width / 4,
                                                             eye_texture_height      / 4};
    const auto preview_window_x1y1 = std::array<uint32_t, 2>{static_cast<uint32_t>(desktop_width  - preview_extents.at(0) ) / 2,
                                                             static_cast<uint32_t>(desktop_height - preview_extents.at(1) ) / 2};
 
    m_preview_window_ptr->set_position(preview_window_x1y1,
                                       preview_extents);

    /* Make sure Q1 window is set as a foreground window. Even though the window is now hidden, this is required
     * for libovr to correctly unmute audio playback! In the event the user alt-tabs out of the app or fiddles
     * with the UI visible in the preview window, we also make the same call in the preview window's rendering
     * thread.
     *
     * Note that this is also required for keypresses to be correctly captured by the app.
     */
    ::SetForegroundWindow(m_q1_hwnd);
}