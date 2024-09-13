/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "VRPlayer.h"
#include "VRPlayer_frame.h"
#include "VRPlayer_preview_window.h"
#include "VRPlayer_preview_window_ui.h"
#include "VRPlayer_vr_renderer.h"
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include "OpenGL/globals.h"
#include "WGL/globals.h"
#include <assert.h>
#include "common_defines.inl"

#ifdef min
    #undef min
#endif


static void glfw_error_callback(int         error,
                                const char* description)
{
    assert(false && "GLFW error callback received");
}


PreviewWindow::PreviewWindow(VRPlayer*    in_vr_player_ptr,
                             IVRPlayback* in_vr_playback_ptr,
                             VRRenderer*  in_vr_renderer_ptr)
    :m_current_frame_ptr       (nullptr),
     m_frame_counter           (0),
     m_preview_fb_id           {},
     m_vr_player_ptr           (in_vr_player_ptr),
     m_vr_playback_ptr         (in_vr_playback_ptr),
     m_vr_renderer_ptr         (in_vr_renderer_ptr),
     m_window_extents_incl_ui  ({0, 0}),
     m_window_extents_wo_ui    ({0, 0}),
     m_window_ptr              (nullptr),
     m_window_x1y1             ({0, 0}),
     m_worked_thread_done_event(0),
     m_worker_thread_must_die  (false),
     m_worked_thread_wait_event(0)
{
    /* Stub */
}

PreviewWindow::~PreviewWindow()
{
    m_worker_thread_must_die = true;

    // Wake up the rendering thread
    ::SetEvent(m_worked_thread_wait_event);

    // Wait for it to die..
    m_worker_thread.join();

    // Clean up.
    m_ui_ptr.reset();

    ::CloseHandle(m_worked_thread_done_event);
    ::CloseHandle(m_worked_thread_wait_event);
}

PreviewWindowUniquePtr PreviewWindow::create(VRPlayer*    in_vr_player_ptr,
                                             IVRPlayback* in_vr_playback_ptr,
                                             VRRenderer*  in_vr_renderer_ptr,
                                             Settings*    in_settings_ptr)
{
    PreviewWindowUniquePtr result_ptr(new PreviewWindow(in_vr_player_ptr,
                                                        in_vr_playback_ptr,
                                                        in_vr_renderer_ptr) );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init(in_settings_ptr) )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

void PreviewWindow::execute()
{
    // This function lives in a dedicated thread.
    int result = 1;

    APIInterceptor::disable_callbacks_for_this_thread            ();
    APIInterceptor::g_logger_ptr->disable_logging_for_this_thread();

    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit() )
    {
        assert(false);

        goto end;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_DEPTH_BITS,            32);
    glfwWindowHint(GLFW_DOUBLEBUFFER,          1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);

    #if defined(_DEBUG)
    {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE,       GLFW_OPENGL_COMPAT_PROFILE);
    }
    #endif

    // Create window with graphics context
    m_window_ptr = glfwCreateWindow(m_window_extents_incl_ui.at(0),
                                    m_window_extents_incl_ui.at(1),
                                    "Q1 VR player",
                                    nullptr,  /* monitor */
                                    nullptr); /* share   */

    if (m_window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    glfwSetWindowPos            (m_window_ptr,
                                 m_window_x1y1.at(0),
                                 m_window_x1y1.at(1) );
    glfwSetWindowRefreshCallback(m_window_ptr,
                                 [](GLFWwindow* /* in_window_ptr */) { glfwPostEmptyEvent(); });

    glfwMakeContextCurrent(m_window_ptr);
    glfwSwapInterval      (0); // Disable vsync - this is handled by libovr.

    // Initialize VR support
    if (!m_vr_playback_ptr->setup_for_bound_gl_context(m_window_extents_wo_ui) )
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    if (!m_vr_renderer_ptr->setup_for_bound_context() )
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    // Initialize UI
    m_ui_ptr->setup_for_bound_context(m_window_ptr);

    /* Set up a framebuffer for the mirror texture. */
    {
        const auto mirror_texture_gl_id = m_vr_playback_ptr->get_preview_texture_gl_id();

        auto pfn_gl_bind_framebuffer       = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_bind_framebuffer);
        auto pfn_gl_framebuffer_texture_2d = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(OpenGL::g_cached_gl_framebuffer_texture_2D);
        auto pfn_gl_gen_framebuffers       = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>     (OpenGL::g_cached_gl_gen_framebuffers);

        pfn_gl_gen_framebuffers      (1,
                                     &m_preview_fb_id);

        AI_ASSERT(m_preview_fb_id != 0);

        pfn_gl_bind_framebuffer      (GL_DRAW_FRAMEBUFFER,
                                      m_preview_fb_id);
        pfn_gl_framebuffer_texture_2d(GL_DRAW_FRAMEBUFFER,
                                      GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D,
                                      mirror_texture_gl_id,
                                      0);
        pfn_gl_bind_framebuffer      (GL_DRAW_FRAMEBUFFER,
                                      0);
    }

    // Main loop
    while (!glfwWindowShouldClose(m_window_ptr) &&
           !m_worker_thread_must_die)
    {
        // Wait for frame data to arrive..
        if (::WaitForSingleObject(m_worked_thread_wait_event,
                                  INFINITE) != WAIT_OBJECT_0)
        {
            AI_ASSERT_FAIL();
    
            goto end;
        }

        if (m_worker_thread_must_die)
        {
            break;
        }

        // To ensure Q1 window stays a foreground window all the time, set it as one here.
        //
        // This necessary for two things:
        //
        // 1) Libovr-driven audio playback to remain uninterrupted. The library does not expose any
        //    nice way of unmuting in the event the process where libovr lives does not have its rendering
        //    window in focus.
        // 2) Key / mouse captures to work flawlessly without resorting to hooks.
        ::SetForegroundWindow(m_vr_player_ptr->get_q1_hwnd() );

        ::glfwPollEvents();

        // Render VR frames.
        AI_ASSERT(m_current_frame_ptr != nullptr);

        if (!m_vr_renderer_ptr->render(m_current_frame_ptr) )
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        // Render the frame.
        {
            auto pfn_gl_bind_framebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(OpenGL::g_cached_gl_bind_framebuffer);
            auto pfn_gl_blit_framebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(OpenGL::g_cached_gl_blit_framebuffer);

            pfn_gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER,
                                    0);
            pfn_gl_bind_framebuffer(GL_READ_FRAMEBUFFER,
                                    m_preview_fb_id);

            pfn_gl_blit_framebuffer(0,                                                          /* srcX0 */
                                    m_window_extents_wo_ui.at(1),                               /* srcY0 */
                                    m_window_extents_wo_ui.at(0),                               /* srcX1 */
                                    0,                                                          /* srcY1 */
                                    0,                                                          /* dstX0 */
                                    m_ui_ptr->get_height_px  (),                                /* dstY0 */
                                    m_window_extents_wo_ui.at(0),                               /* dstX1 */
                                    m_window_extents_wo_ui.at(1) + m_ui_ptr->get_height_px(),   /* dstY1 */
                                    GL_COLOR_BUFFER_BIT,
                                    GL_NEAREST);
        }

        // Render UI
        m_ui_ptr->render(m_window_extents_wo_ui.at(1),
                         m_window_extents_wo_ui.at(0) );

        /* All done */
        glfwSwapBuffers(m_window_ptr);

        // Ping the main thread back, indicating the frame has been processed.
        if (::SetEvent(m_worked_thread_done_event) == 0)
        {
            // oh no!
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    m_vr_playback_ptr->deinit_for_bound_gl_context();

end:
    glfwDestroyWindow(m_window_ptr);
    glfwTerminate    ();
}

bool PreviewWindow::init(Settings* in_settings_ptr)
{
    m_ui_ptr = PreviewWindowUI::create(in_settings_ptr);
    AI_ASSERT(m_ui_ptr != nullptr);

    m_worked_thread_done_event = ::CreateEvent(nullptr, /* lpEventAttributes */
                                               FALSE,   /* bManualReset      */
                                               FALSE,   /* bInitialState     */
                                               nullptr);
    m_worked_thread_wait_event = ::CreateEvent(nullptr, /* lpEventAttributes */
                                               FALSE,   /* bManualReset      */
                                               FALSE,   /* bInitialState     */
                                               nullptr);

    AI_ASSERT(m_worked_thread_done_event != 0);
    AI_ASSERT(m_worked_thread_wait_event != 0);

    return (m_ui_ptr                   != nullptr) &&
           (m_worked_thread_done_event != 0)       &&
           (m_worked_thread_wait_event != 0);
}

void PreviewWindow::on_frame_available(const Frame* in_frame_ptr) const
{
    // NOTE: This function is called from application's rendering thread.
    AI_ASSERT(in_frame_ptr->get_n_api_commands() != 0);

    m_current_frame_ptr = in_frame_ptr;

    // Wake up our rendering thread..
    if (::SetEvent(m_worked_thread_wait_event) == 0)
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    // Now wait for it to ping us back, indicating the frame has been processed.
    if (::WaitForSingleObject(m_worked_thread_done_event,
                              INFINITE) != WAIT_OBJECT_0)
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    m_current_frame_ptr = nullptr;
    m_frame_counter     ++;

end:
    ;
}

void PreviewWindow::set_position(const std::array<uint32_t, 2>& in_x1y1,
                                 const std::array<uint32_t, 2>& in_extents)
{
    m_window_extents_wo_ui = in_extents;
    m_window_x1y1          = in_x1y1;

    /* NOTE: It is crucial to extend the window height to account for space needed to render UI controls! */
    m_window_extents_incl_ui        = m_window_extents_wo_ui;
    m_window_extents_incl_ui.at(1) += m_ui_ptr->get_height_px();

    if (m_window_ptr == nullptr)
    {
        /* NOTE: The worker thread uses m_window_* fields when creating the window for the first time. */
        m_worker_thread = std::thread(&PreviewWindow::execute,
                                       this);
    }
    else
    {
        /* TODO: Implement if needed */
        AI_ASSERT_FAIL();
#if 0
        glfwSetWindowPos(m_window_ptr,
                         in_x1y1.at    (0),
                         in_x1y1.at    (1) );
        glfwSetWindowSize(m_window_ptr,
                          in_extents.at(0),
                          in_extents.at(1) );
#endif
    }
}
