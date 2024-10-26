/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_defines.inl"
#include "VRPlayer_frame.h"
#include "VRPlayer_frame_player.h"
#include "VRPlayer_vr_renderer.h"
#include "OpenGL/globals.h"
#include "WGL/globals.h"

VRRenderer::VRRenderer(FramePlayer* in_frame_player_ptr,
                       IVRPlayback* in_vr_playback_ptr)
    :m_eye0_color_fb_id(0),
     m_eye1_color_fb_id(0),
     m_frame_player_ptr(in_frame_player_ptr),
     m_ui_fb_id        (0),
     m_vr_playback_ptr (in_vr_playback_ptr)

{
    /* Stub */
}

VRRenderer::~VRRenderer()
{
    /* TODO: Proper resource release. We don't really care that much anyway. */
}

VRRendererUniquePtr VRRenderer::create(FramePlayer* in_frame_player_ptr,
                                       IVRPlayback* in_vr_playback_ptr)
{
    VRRendererUniquePtr result_ptr;

    result_ptr.reset(new VRRenderer(in_frame_player_ptr,
                                    in_vr_playback_ptr));

    AI_ASSERT(result_ptr != nullptr);
    return result_ptr;
}

bool VRRenderer::render(const Frame* in_frame_ptr)
{
    bool result = false;

    // This function is called from preview window's rendering thread.
    const auto is_ui_texture_supported = m_vr_playback_ptr->supports_separate_ui_texture();

    for (bool is_left_eye : {true, false})
    {
        uint32_t eye_texture_id      = 0;
        uint32_t eye_texture_n_layer = 0;
        uint32_t ui_texture_id       = 0;
        uint32_t ui_texture_n_layer  = UINT32_MAX;

        if (!m_vr_playback_ptr->acquire_eye_texture(is_left_eye,
                                                   &eye_texture_id,
                                                   &eye_texture_n_layer,
                                                   &ui_texture_id,
                                                   &ui_texture_n_layer) )
        {
            AI_ASSERT_FAIL();

            goto end;
        }

        render_eye_frames(is_left_eye,
                          (eye_texture_id != UINT32_MAX) ? eye_texture_id :0,
                          (is_ui_texture_supported) ? &eye_texture_n_layer : nullptr,
                          (is_ui_texture_supported) ? &ui_texture_id       : nullptr,
                          (is_ui_texture_supported) ? &ui_texture_n_layer  : nullptr,
                          in_frame_ptr);

        if (!m_vr_playback_ptr->commit_eye_texture() )
        {
            AI_ASSERT_FAIL();

            goto end;
        }
    }

    if (!m_vr_playback_ptr->present() )
    {
        AI_ASSERT_FAIL();

        goto end;
    }

    result = true;
end:
    return result;
}

void VRRenderer::render_eye_frames(const bool&     in_left_eye,
                                   const uint32_t& in_eye_texture_gl_id,
                                   const uint32_t* in_opt_eye_texture_n_layer_ptr,
                                   const uint32_t* in_opt_ui_texture_gl_id_ptr,
                                   const uint32_t* in_opt_ui_texture_n_layer_ptr,
                                   const Frame*    in_frame_ptr)
{
    // This function is called from preview window's rendering thread.
    auto pfn_wgl_get_proc_address_func = reinterpret_cast<WGL::PFNWGLGETPROCADDRESSPROC>(WGL::g_cached_get_proc_address_func_ptr);

    auto pfn_gl_alpha_func            = reinterpret_cast<PFNGLALPHAFUNCPROC>           (OpenGL::g_cached_gl_alpha_func);
    auto pfn_gl_bind_framebuffer      = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_bind_framebuffer);
    auto pfn_gl_bind_texture          = reinterpret_cast<PFNGLBINDTEXTUREPROC>         (OpenGL::g_cached_gl_bind_texture);
    auto pfn_gl_blend_func            = reinterpret_cast<PFNGLBLENDFUNCPROC>           (OpenGL::g_cached_gl_blend_func);
    auto pfn_gl_clear                 = reinterpret_cast<PFNGLCLEARPROC>               (OpenGL::g_cached_gl_clear);
    auto pfn_gl_clear_color           = reinterpret_cast<PFNGLCLEARCOLORPROC>          (OpenGL::g_cached_gl_clear_color);
    auto pfn_gl_clear_depth           = reinterpret_cast<PFNGLCLEARDEPTHPROC>          (OpenGL::g_cached_gl_clear_depth);
    auto pfn_gl_cull_face             = reinterpret_cast<PFNGLCULLFACEPROC>            (OpenGL::g_cached_gl_cull_face);
    auto pfn_gl_depth_func            = reinterpret_cast<PFNGLDEPTHFUNCPROC>           (OpenGL::g_cached_gl_depth_func);
    auto pfn_gl_depth_mask            = reinterpret_cast<PFNGLDEPTHMASKPROC>           (OpenGL::g_cached_gl_depth_mask);
    auto pfn_gl_depth_range           = reinterpret_cast<PFNGLDEPTHRANGEPROC>          (OpenGL::g_cached_gl_depth_range);
    auto pfn_gl_disable               = reinterpret_cast<PFNGLDISABLEPROC>             (OpenGL::g_cached_gl_disable);
    auto pfn_gl_enable                = reinterpret_cast<PFNGLENABLEPROC>              (OpenGL::g_cached_gl_enable);
    auto pfn_gl_front_face            = reinterpret_cast<PFNGLFRONTFACEPROC>           (OpenGL::g_cached_gl_front_face);
    auto pfn_gl_gen_textures          = reinterpret_cast<PFNGLGENTEXTURESPROC>         (OpenGL::g_cached_gl_gen_textures);
    auto pfn_gl_get_floatv            = reinterpret_cast<PFNGLGETFLOATVPROC>           (OpenGL::g_cached_gl_get_floatv);
    auto pfn_gl_load_matrix_d         = reinterpret_cast<PFNGLLOADMATRIXDPROC>         (OpenGL::g_cached_gl_load_matrix_d);
    auto pfn_gl_load_matrix_f         = reinterpret_cast<PFNGLLOADMATRIXFPROC>         (OpenGL::g_cached_gl_load_matrix_f);
    auto pfn_gl_matrix_mode           = reinterpret_cast<PFNGLMATRIXMODEPROC>          (OpenGL::g_cached_gl_matrix_mode);
    auto pfn_gl_polygon_mode          = reinterpret_cast<PFNGLPOLYGONMODEPROC>         (OpenGL::g_cached_gl_polygon_mode);
    auto pfn_gl_read_buffer           = reinterpret_cast<PFNGLREADBUFFERPROC>          (OpenGL::g_cached_gl_read_buffer);
    auto pfn_gl_shade_model           = reinterpret_cast<PFNGLSHADEMODELPROC>          (OpenGL::g_cached_gl_shade_model);
    auto pfn_gl_tex_envf              = reinterpret_cast<PFNGLTEXENVFPROC>             (OpenGL::g_cached_gl_tex_env_f);
    auto pfn_gl_viewport              = reinterpret_cast<PFNGLVIEWPORTPROC>            (OpenGL::g_cached_gl_viewport);

    // Update framebuffer color attachments to use the specified textures.
    const auto& color_fb_id = (in_left_eye) ? m_eye0_color_fb_id
                                            : m_eye1_color_fb_id;

    AI_ASSERT(glGetError() == GL_NO_ERROR);

    if ( in_opt_ui_texture_gl_id_ptr != nullptr    &&
        *in_opt_ui_texture_gl_id_ptr != UINT32_MAX)
    {
        auto pfn_gl_framebuffer_texturelayer = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURELAYERPROC>(OpenGL::g_cached_gl_framebuffer_texture_layer);

        pfn_gl_bind_framebuffer        (GL_DRAW_FRAMEBUFFER,
                                        m_ui_fb_id);
        pfn_gl_framebuffer_texturelayer(GL_DRAW_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        *in_opt_ui_texture_gl_id_ptr,
                                        0, /* level */
                                        *in_opt_ui_texture_n_layer_ptr);

        pfn_gl_bind_framebuffer        (GL_DRAW_FRAMEBUFFER,
                                        color_fb_id);
        pfn_gl_framebuffer_texturelayer(GL_DRAW_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        in_eye_texture_gl_id,
                                        0, /* level */
                                       *in_opt_eye_texture_n_layer_ptr);
    }
    else
    {
        auto pfn_gl_framebuffer_texture2d = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(OpenGL::g_cached_gl_framebuffer_texture_2D);

        pfn_gl_bind_framebuffer     (GL_DRAW_FRAMEBUFFER,
                                     color_fb_id);
        pfn_gl_framebuffer_texture2d(GL_DRAW_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     in_eye_texture_gl_id,
                                     0); /* level */
    }

    AI_ASSERT(glGetError() == GL_NO_ERROR);

    pfn_gl_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    pfn_gl_clear      (GL_COLOR_BUFFER_BIT);

    m_frame_player_ptr->play(in_frame_ptr,
                             in_left_eye,
                             color_fb_id,
                             (m_ui_fb_id != 0) ? &m_ui_fb_id : nullptr);

    /* Everything OK? */
    AI_ASSERT(glGetError() == GL_NO_ERROR);
}

bool VRRenderer::setup_for_bound_context()
{
    // This function is called from preview window's rendering thread.
    bool result = false;

    /* Set up FBs we will be using to render the frame from two different eye locations, if this is the first time this
     * function is called.
     */
    auto pfn_gl_bind_framebuffer      = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>     (OpenGL::g_cached_gl_bind_framebuffer);
    auto pfn_gl_bind_texture          = reinterpret_cast<PFNGLBINDTEXTUREPROC>         (OpenGL::g_cached_gl_bind_texture);
    auto pfn_gl_draw_buffer           = reinterpret_cast<PFNGLDRAWBUFFERPROC>          (OpenGL::g_cached_gl_draw_buffer);
    auto pfn_gl_enable                = reinterpret_cast<PFNGLENABLEPROC>              (OpenGL::g_cached_gl_enable);
    auto pfn_gl_framebuffer_texture2d = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(OpenGL::g_cached_gl_framebuffer_texture_2D);
    auto pfn_gl_gen_framebuffers      = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>     (OpenGL::g_cached_gl_gen_framebuffers);
    auto pfn_gl_tex_image_2d          = reinterpret_cast<PFNGLTEXIMAGE2DPROC>          (OpenGL::g_cached_gl_tex_image_2D);
    auto pfn_gl_tex_parameteri        = reinterpret_cast<PFNGLTEXPARAMETERIPROC>       (OpenGL::g_cached_gl_tex_parameteri);

    pfn_gl_enable(GL_TEXTURE_2D);

    if (m_eye0_color_fb_id == 0)
    {
        GLuint fb_ids     [3]{};
        GLuint texture_ids[4]{};

        {
            m_eye0_depth_texture_id = 31338;
            m_eye1_depth_texture_id = 54322;

            for (uint32_t n_eye = 0;
                          n_eye < 2;
                        ++n_eye)
            {
                const auto& depth_texture_id       = (n_eye == 0) ? m_eye0_depth_texture_id
                                                                  : m_eye1_depth_texture_id;
                const auto& eye_texture_resolution = m_vr_playback_ptr->get_eye_texture_resolution(n_eye == 0); /* in_left_eye */

                {
                    const auto texture_id             = depth_texture_id;
                    const auto texture_format         = GL_DEPTH_COMPONENT;
                    const auto texture_internalformat = GL_DEPTH_COMPONENT32F;
                    const auto texture_type           = GL_FLOAT;

                    pfn_gl_bind_texture(GL_TEXTURE_2D,
                                        texture_id);
                    pfn_gl_tex_image_2d(GL_TEXTURE_2D,
                                        0, /* level */
                                        texture_internalformat,
                                        eye_texture_resolution.at(0),
                                        eye_texture_resolution.at(1),
                                        0, /* border */
                                        texture_format,
                                        texture_type,
                                        nullptr); /* pixels */

                    pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                                          GL_TEXTURE_MAG_FILTER,
                                          GL_LINEAR);
                    pfn_gl_tex_parameteri(GL_TEXTURE_2D,
                                          GL_TEXTURE_MIN_FILTER,
                                          GL_LINEAR);
                }
            }
        }

        {
            const auto ui_fb_needed = m_vr_playback_ptr->supports_separate_ui_texture();
            uint32_t   n_fbs_needed = (ui_fb_needed) ? 3u
                                                     : 2u;

            pfn_gl_gen_framebuffers(n_fbs_needed,
                                    fb_ids);

            AI_ASSERT(fb_ids[0] != 0);
            AI_ASSERT(fb_ids[1] != 0);

            for (uint32_t n_fb_id = 0;
                          n_fb_id < n_fbs_needed;
                        ++n_fb_id)
            {
                static const GLenum gl_color_attachment0_enum = GL_COLOR_ATTACHMENT0;
                const auto&         depth_texture_id          = (n_fb_id == 0) ? m_eye0_depth_texture_id
                                                              : (n_fb_id == 1) ? m_eye1_depth_texture_id
                                                                               : 0u;

                pfn_gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER,
                                        fb_ids[n_fb_id]);

                if (depth_texture_id != 0 &&
                    n_fb_id          <  2)
                {
                    pfn_gl_framebuffer_texture2d(GL_DRAW_FRAMEBUFFER,
                                                 GL_DEPTH_ATTACHMENT,
                                                 GL_TEXTURE_2D,
                                                 depth_texture_id,
                                                 0); /* level */
                }

                pfn_gl_draw_buffer(GL_COLOR_ATTACHMENT0);
            }

            m_eye0_color_fb_id = fb_ids[0];
            m_eye1_color_fb_id = fb_ids[1];

            if (ui_fb_needed)
            {
                m_ui_fb_id = fb_ids[2];
            }
        }
    }
    else
    {
        AI_ASSERT(m_eye0_depth_texture_id != 0);
        AI_ASSERT(m_eye1_color_fb_id      != 0);
        AI_ASSERT(m_eye1_depth_texture_id != 0);

        if (m_vr_playback_ptr->supports_separate_ui_texture() )
        {
            AI_ASSERT(m_ui_fb_id != 0);
        }
    }

    result = true;
    return result;
}