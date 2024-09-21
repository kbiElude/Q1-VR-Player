/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "common_defines.inl"
#include "VRPlayer_frame.h"
#include "VRPlayer_frame_player.h"
#include "VRPlayer_settings.h"
#include "OpenGL/globals.h"
#include "WGL/globals.h"

#ifdef min
    #undef min
#endif


FramePlayer::FramePlayer(const IVRPlayback* in_vr_playback_ptr,
                         const Settings*    in_settings_ptr)
    :m_settings_ptr   (in_settings_ptr),
     m_vr_playback_ptr(in_vr_playback_ptr)
{
    /* Stub */
}

FramePlayer::~FramePlayer()
{
    /* Stub */
}

FramePlayerUniquePtr FramePlayer::create(const IVRPlayback* in_vr_playback_ptr,
                                         const Settings*    in_settings_ptr)
{
    FramePlayerUniquePtr result_ptr;

    result_ptr.reset(new FramePlayer(in_vr_playback_ptr,
                                     in_settings_ptr) );

    AI_ASSERT(result_ptr != nullptr);
    return result_ptr;
}

void FramePlayer::play(const Frame* in_frame_ptr,
                       const bool&  in_left_eye)
{
    auto pfn_wgl_get_proc_address_func = reinterpret_cast<WGL::PFNWGLGETPROCADDRESSPROC>(WGL::g_cached_get_proc_address_func_ptr);

    auto pfn_gl_alpha_func            = reinterpret_cast<PFNGLALPHAFUNCPROC>           (OpenGL::g_cached_gl_alpha_func);
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
    auto pfn_gl_load_identity         = reinterpret_cast<PFNGLLOADIDENTITYPROC>        (OpenGL::g_cached_gl_load_identity);
    auto pfn_gl_load_matrix_d         = reinterpret_cast<PFNGLLOADMATRIXDPROC>         (OpenGL::g_cached_gl_load_matrix_d);
    auto pfn_gl_matrix_mode           = reinterpret_cast<PFNGLMATRIXMODEPROC>          (OpenGL::g_cached_gl_matrix_mode);
    auto pfn_gl_polygon_mode          = reinterpret_cast<PFNGLPOLYGONMODEPROC>         (OpenGL::g_cached_gl_polygon_mode);
    auto pfn_gl_pop_matrix            = reinterpret_cast<PFNGLPOPMATRIXPROC>           (OpenGL::g_cached_gl_pop_matrix);
    auto pfn_gl_push_matrix           = reinterpret_cast<PFNGLPUSHMATRIXPROC>          (OpenGL::g_cached_gl_push_matrix);
    auto pfn_gl_shade_model           = reinterpret_cast<PFNGLSHADEMODELPROC>          (OpenGL::g_cached_gl_shade_model);
    auto pfn_gl_tex_envf              = reinterpret_cast<PFNGLTEXENVFPROC>             (OpenGL::g_cached_gl_tex_env_f);
    auto pfn_gl_viewport              = reinterpret_cast<PFNGLVIEWPORTPROC>            (OpenGL::g_cached_gl_viewport);

    // NOTE: Projection matrix used by Q1 needs to be replaced to account for different aspect ratio
    //       used by the VR system. To do that, we need to keep track of a few states.
    bool                  is_console_texture_bound          = false;
    bool                  is_ortho_enabled                  = false;
    bool                  is_status_bar_band_texture_bound  = false;
    bool                  is_texture_2d_enabled             = false;
    bool                  status_bar_rendered               = false;
    std::array<double, 2> ortho_x1y1;
    std::array<double, 2> ortho_x2y2;

    const uint32_t q1_console_texture_id         = 2;
    const uint32_t q1_status_bar_band_texture_id = 6;
    const uint32_t q1_status_bar_height          = 32;
    const uint32_t q1_hud_height                 = 480;
    const uint32_t q1_hud_width                  = 640;

    const auto& eye_offset_x     = m_vr_playback_ptr->get_eye_offset_x          (in_left_eye) * m_settings_ptr->get_eye_separation_multiplier();
    const auto& viewport_extents = m_vr_playback_ptr->get_eye_texture_resolution(in_left_eye);

    {
        const auto n_api_commands  = in_frame_ptr->get_n_api_commands ();
        auto       start_state_ptr = in_frame_ptr->get_start_state_ptr();

        {
            bool        glrotate_called      = false;
            bool        pitch_angle_set      = false;
            bool        yaw_angle_set        = false;
            const auto  should_init_textures = (in_left_eye);

            {
                struct
                {
                    GLenum cap;
                    bool   state;
                } states[] =
                {
                    {GL_ALPHA_TEST,   start_state_ptr->is_alpha_test_enabled},
                    {GL_BLEND,        start_state_ptr->is_blend_enabled},
                    {GL_CULL_FACE,    start_state_ptr->is_cull_face_enabled},
                    {GL_DEPTH_TEST,   start_state_ptr->is_depth_test_enabled},
                    {GL_SCISSOR_TEST, start_state_ptr->is_scissor_test_enabled},
                    {GL_TEXTURE_2D,   start_state_ptr->is_texture_2d_enabled},
                };

                for (const auto& current_state : states)
                {
                    if (current_state.state)
                    {
                        pfn_gl_enable(current_state.cap);
                    }
                    else
                    {
                        pfn_gl_disable(current_state.cap);
                    }
                }
            }

            pfn_gl_matrix_mode  (GL_MODELVIEW);
            pfn_gl_load_matrix_d(start_state_ptr->modelview_matrix);
            pfn_gl_matrix_mode  (GL_PROJECTION);
            pfn_gl_load_matrix_d(start_state_ptr->projection_matrix);

            pfn_gl_alpha_func      (start_state_ptr->alpha_func_func,
                                    start_state_ptr->alpha_func_ref);
            pfn_gl_bind_texture    (GL_TEXTURE_2D,
                                    start_state_ptr->bound_texture_2d_gl_id);
            pfn_gl_blend_func      (start_state_ptr->blend_func_sfactor,
                                    start_state_ptr->blend_func_dfactor);
            pfn_gl_clear_color     (start_state_ptr->clear_color[0],
                                    start_state_ptr->clear_color[1],
                                    start_state_ptr->clear_color[2],
                                    start_state_ptr->clear_color[3]);
            pfn_gl_clear_depth     (start_state_ptr->clear_depth);
            pfn_gl_cull_face       (start_state_ptr->cull_face_mode);
            pfn_gl_depth_func      (start_state_ptr->depth_func);
            pfn_gl_depth_mask      (start_state_ptr->depth_mask);
            pfn_gl_depth_range     (start_state_ptr->depth_range[0],
                                    start_state_ptr->depth_range[1]);
            pfn_gl_front_face      (start_state_ptr->front_face_mode);
            pfn_gl_matrix_mode     (start_state_ptr->matrix_mode);
            pfn_gl_polygon_mode    (GL_BACK,
                                    start_state_ptr->polygon_mode_back_mode);
            pfn_gl_polygon_mode    (GL_FRONT,
                                    start_state_ptr->polygon_mode_front_mode);
            pfn_gl_shade_model     (start_state_ptr->shade_model);
            pfn_gl_tex_envf        (GL_TEXTURE_ENV,
                                    GL_TEXTURE_ENV_MODE,
                                    static_cast<float>(start_state_ptr->texture_env_mode) );
            pfn_gl_viewport        (0,
                                    0,
                                    viewport_extents.at(0),
                                    viewport_extents.at(1) );

            for (uint32_t n_api_command = 0;
                          n_api_command < n_api_commands;
                        ++n_api_command)
            {
                const auto api_command_ptr = in_frame_ptr->get_api_command_ptr(n_api_command);

                AI_ASSERT(api_command_ptr != nullptr);

                if (api_command_ptr->api_func == APIInterceptor::APIFUNCTION_GL_GLDRAWBUFFER)
                {
                    /* Q1 force-draws into front buffer when loading assets but by doing so it would
                     * wreak havoc to our rendering flow, since we're rendering to an off-screen texture.
                     * Drop this call. */
                     continue;
                }

                if (api_command_ptr->api_func == APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D)
                {
                    /* Assume mips of a given texture are never initialized for a single frame more than once.
                     * This means we only need to execute the command when rendering left eye's frame.
                     */
                    if (!should_init_textures)
                    {
                        continue;
                    }
                }

                // Execute the command.
                switch (api_command_ptr->api_func)
                {
                    case APIInterceptor::APIFUNCTION_GL_GLALPHAFUNC:
                    {
                        reinterpret_cast<PFNGLALPHAFUNCPROC>(OpenGL::g_cached_gl_alpha_func)(api_command_ptr->args[0].get_u32 (),
                                                                                             api_command_ptr->args[1].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLBEGIN:
                    {
                        reinterpret_cast<PFNGLBEGINPROC>(OpenGL::g_cached_gl_begin)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE:
                    {
                        const auto game_context_texture_id = api_command_ptr->args[1].get_u32();
                        GLuint     this_context_texture_id = 0;

                        /* NOTE: Console window is always rendered as last. */
                        is_console_texture_bound         |= (game_context_texture_id == q1_console_texture_id);
                        is_status_bar_band_texture_bound  = (game_context_texture_id == q1_status_bar_band_texture_id);

                        status_bar_rendered |= is_status_bar_band_texture_bound;

                        if (game_context_texture_id != 0)
                        {
                            auto map_iterator = m_game_to_this_context_texture_gl_id_map.find(game_context_texture_id);

                            if (map_iterator == m_game_to_this_context_texture_gl_id_map.end())
                            {
                                uint32_t new_gl_id = 0;

                                pfn_gl_gen_textures(1,
                                                   &new_gl_id);

                                m_game_to_this_context_texture_gl_id_map[game_context_texture_id] = new_gl_id;
                                this_context_texture_id                                           = new_gl_id;
                            }
                            else
                            {
                                this_context_texture_id = map_iterator->second;
                            }
                        }

                        reinterpret_cast<PFNGLBINDTEXTUREPROC>(OpenGL::g_cached_gl_bind_texture)(api_command_ptr->args[0].get_u32(),
                                                                                                 this_context_texture_id);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLBLENDFUNC:
                    {
                        reinterpret_cast<PFNGLBLENDFUNCPROC>(OpenGL::g_cached_gl_blend_func)(api_command_ptr->args[0].get_u32(),
                                                                                             api_command_ptr->args[1].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCLEAR:
                    {
                        reinterpret_cast<PFNGLCLEARPROC>(OpenGL::g_cached_gl_clear)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCLEARCOLOR:
                    {
                        reinterpret_cast<PFNGLCLEARCOLORPROC>(OpenGL::g_cached_gl_clear_color)(api_command_ptr->args[0].get_fp32(),
                                                                                               api_command_ptr->args[1].get_fp32(),
                                                                                               api_command_ptr->args[2].get_fp32(),
                                                                                               api_command_ptr->args[3].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCLEARDEPTH:
                    {
                        reinterpret_cast<PFNGLCLEARDEPTHPROC>(OpenGL::g_cached_gl_clear_depth)(api_command_ptr->args[0].get_fp64() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCOLOR3F:
                    {
                        const float color[3] =
                        {
                            api_command_ptr->args[0].get_fp32(),
                            api_command_ptr->args[1].get_fp32(),
                            api_command_ptr->args[2].get_fp32()
                        };

                        reinterpret_cast<PFNGLCOLOR3FPROC>(OpenGL::g_cached_gl_color_3f)(color[0],
                                                                                         color[1],
                                                                                         color[2]);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCOLOR3UB:
                    {
                        reinterpret_cast<PFNGLCOLOR3UBPROC>(OpenGL::g_cached_gl_color_3ub)(api_command_ptr->args[0].get_u8(),
                                                                                           api_command_ptr->args[1].get_u8(),
                                                                                           api_command_ptr->args[2].get_u8() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCOLOR4F:
                    {
                        const float color[4] =
                        {
                            api_command_ptr->args[0].get_fp32(),
                            api_command_ptr->args[1].get_fp32(),
                            api_command_ptr->args[2].get_fp32(),
                            api_command_ptr->args[3].get_fp32()
                        };

                        reinterpret_cast<PFNGLCOLOR4FPROC>(OpenGL::g_cached_gl_color_4f)(color[0],
                                                                                         color[1],
                                                                                         color[2],
                                                                                         color[3]);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLCULLFACE:
                    {
                        reinterpret_cast<PFNGLCULLFACEPROC>(OpenGL::g_cached_gl_cull_face)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLDEPTHFUNC:
                    {
                        reinterpret_cast<PFNGLDEPTHFUNCPROC>(OpenGL::g_cached_gl_depth_func)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLDEPTHMASK:
                    {
                        reinterpret_cast<PFNGLDEPTHMASKPROC>(OpenGL::g_cached_gl_depth_mask)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLDEPTHRANGE:
                    {
                        reinterpret_cast<PFNGLDEPTHRANGEPROC>(OpenGL::g_cached_gl_depth_range)(api_command_ptr->args[0].get_fp64(),
                                                                                               api_command_ptr->args[1].get_fp64() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLDISABLE:
                    {
                        const auto cap = api_command_ptr->args[0].get_u32();

                        if (cap == GL_TEXTURE_2D)
                        {
                            is_texture_2d_enabled = false;
                        }

                        reinterpret_cast<PFNGLDISABLEPROC>(OpenGL::g_cached_gl_disable)(cap);
                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLENABLE:
                    {
                        const auto cap = api_command_ptr->args[0].get_u32();

                        if (cap == GL_TEXTURE_2D)
                        {
                            is_texture_2d_enabled = true;
                        }

                        reinterpret_cast<PFNGLENABLEPROC>(OpenGL::g_cached_gl_enable)(cap);
                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLEND:
                    {
                        reinterpret_cast<PFNGLENDPROC>(OpenGL::g_cached_gl_end)();

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLFINISH:
                    {
                        // No need to replay this.

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLFLUSH:
                    {
                        // No need to replay this.

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLFRONTFACE:
                    {
                        reinterpret_cast<PFNGLFRONTFACEPROC>(OpenGL::g_cached_gl_front_face)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLFRUSTUM:
                    {
                        // Q1 uses glFrustum() to set up projection matrix. Use VR-specific values to calculate frustum coords
                        // taking into account the aspect ratio of the eye texture. No need to adjust Z range.
                        const auto  down_tan     = m_vr_playback_ptr->get_tan_between_view_vec_and_bottom_fov_edge(in_left_eye);
                        const auto  up_tan       = m_vr_playback_ptr->get_tan_between_view_vec_and_top_fov_edge   (in_left_eye);
                        const float aspect_ratio = static_cast<float>                                             (viewport_extents.at(0) ) / static_cast<float>(viewport_extents.at(1) );
                        double      near_val     = api_command_ptr->args[4].get_fp64                              ();
                        double      far_val      = api_command_ptr->args[5].get_fp64                              ();

                        double top    =  near_val     * down_tan;
                        double bottom = -near_val     * up_tan;
                        double right  =  aspect_ratio * top;
                        double left   = -right;

                        reinterpret_cast<PFNGLFRUSTUMPROC>(OpenGL::g_cached_gl_frustum)(left,
                                                                                        right,
                                                                                        bottom,
                                                                                        top,
                                                                                        near_val,
                                                                                        far_val);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLLOADIDENTITY:
                    {
                        reinterpret_cast<PFNGLLOADIDENTITYPROC>(OpenGL::g_cached_gl_load_identity)();

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLMATRIXMODE:
                    {
                        reinterpret_cast<PFNGLMATRIXMODEPROC>(OpenGL::g_cached_gl_matrix_mode)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLORTHO:
                    {
                        // Just as we did with the perspective matrix, we also need to handle orthogonal projection.
                        //
                        // Quake 1 configures the matrix so that <0, 640>x<0, 480> is mapped onto NDC space. We need to
                        // change this to match the resolution of the eye texture.
                        //
                        // But there's more. Due to barrel distortion, UI is likely not going to fit the way it fits on
                        // a regular 2D screen. This is not much of an issue with 3D geometry because your eyes do not notice this,
                        // but with UI the issue becomes immediately visible due to things like menu options getting chopped off.
                        //
                        // There's no nice way of solving this and we're not going to go & redesign the UI in any fashion.
                        // Instead, we simply scale down the UI to fit in a smaller centered rectangle and expose a few knobs to the user
                        // to tweak as they please.

                        const auto original_left   = api_command_ptr->args[0].get_fp64();
                        const auto original_right  = api_command_ptr->args[1].get_fp64();
                        const auto original_bottom = api_command_ptr->args[2].get_fp64();
                        const auto original_top    = api_command_ptr->args[3].get_fp64();

                        AI_ASSERT(original_left   == 0);
                        AI_ASSERT(original_right  == 640);
                        AI_ASSERT(original_bottom == 480);
                        AI_ASSERT(original_top    == 0);

                        const auto scale    = m_settings_ptr->get_ui_scale();
                        const auto offset_x = static_cast<double>(viewport_extents.at(0) ) * (1.0 - scale) * 0.5;
                        const auto offset_y = static_cast<double>(viewport_extents.at(1) ) * (1.0 - scale) * 0.5;

                        const double new_left   =  offset_x;
                        const double new_right  = -offset_x + static_cast<double>(viewport_extents.at(0) );
                        const double new_bottom = -offset_y + static_cast<double>(viewport_extents.at(1) );
                        const double new_top    =  offset_y;

                        const auto ortho_offset =  m_vr_playback_ptr->get_eye_texture_resolution  (in_left_eye).at(0) *
                                                  -m_vr_playback_ptr->get_eye_offset_x            (in_left_eye)       *
                                                   m_settings_ptr->get_ortho_separation_multiplier();

                        reinterpret_cast<PFNGLORTHOPROC>(OpenGL::g_cached_gl_ortho)(0                      + ortho_offset,
                                                                                    viewport_extents.at(0) + ortho_offset,
                                                                                    viewport_extents.at(1),
                                                                                    0,
                                                                                    api_command_ptr->args[4].get_fp64(),
                                                                                    api_command_ptr->args[5].get_fp64() );

                        is_ortho_enabled = true;
                        ortho_x1y1.at(0) = new_left;
                        ortho_x1y1.at(1) = new_top;
                        ortho_x2y2.at(0) = new_right;
                        ortho_x2y2.at(1) = new_bottom;

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLPOLYGONMODE:
                    {
                        reinterpret_cast<PFNGLPOLYGONMODEPROC>(OpenGL::g_cached_gl_polygon_mode)(api_command_ptr->args[0].get_u32(),
                                                                                                 api_command_ptr->args[1].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLPOPMATRIX:
                    {
                        reinterpret_cast<PFNGLPOPMATRIXPROC>(OpenGL::g_cached_gl_pop_matrix)();

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLPUSHMATRIX:
                    {
                        reinterpret_cast<PFNGLPUSHMATRIXPROC>(OpenGL::g_cached_gl_push_matrix)();

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLREADPIXELS:
                    {
                        // No need to replay this

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLROTATEF:
                    {
                        if (!glrotate_called)
                        {
                            reinterpret_cast<PFNGLTRANSLATEFPROC>(OpenGL::g_cached_gl_translate_f)(eye_offset_x,
                                                                                                   0.0f,
                                                                                                   0.0f);
                            glrotate_called = true;
                        }

                        float angle = api_command_ptr->args[0].get_fp32();
                        float x     = api_command_ptr->args[1].get_fp32();
                        float y     = api_command_ptr->args[2].get_fp32();
                        float z     = api_command_ptr->args[3].get_fp32();

                        if (fabs(x)        < 1e-5f &&
                            fabs(y)        < 1e-5f &&
                            fabs(z - 1.0f) < 1e-5f)
                        {
                            if (!yaw_angle_set)
                            {
                                const float yaw_angle = m_vr_playback_ptr->get_current_yaw_angle();

                                angle         += yaw_angle;
                                yaw_angle_set  = true;
                            }
                        }
                        else
                        if (fabs(x)        < 1e-5f &&
                            fabs(y - 1.0f) < 1e-5f &&
                            fabs(z)        < 1e-5f)
                        {
                            if (!pitch_angle_set)
                            {
                                angle           += m_vr_playback_ptr->get_current_pitch_angle();
                                pitch_angle_set  = true;
                            }
                        }

                        reinterpret_cast<PFNGLROTATEFPROC>(OpenGL::g_cached_gl_rotate_f)(angle,
                                                                                         x,
                                                                                         y,
                                                                                         z);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLSCALEF:
                    {
                        reinterpret_cast<PFNGLSCALEFPROC>(OpenGL::g_cached_gl_scale_f)(api_command_ptr->args[0].get_fp32(),
                                                                                       api_command_ptr->args[1].get_fp32(),
                                                                                       api_command_ptr->args[2].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLSHADEMODEL:
                    {
                        reinterpret_cast<PFNGLSHADEMODELPROC>(OpenGL::g_cached_gl_shade_model)(api_command_ptr->args[0].get_u32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD2F:
                    {
                        reinterpret_cast<PFNGLTEXCOORD2FPROC>(OpenGL::g_cached_gl_tex_coord_2f)(api_command_ptr->args[0].get_fp32(),
                                                                                                api_command_ptr->args[1].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLTEXENVF:
                    {
                        reinterpret_cast<PFNGLTEXENVFPROC>(OpenGL::g_cached_gl_tex_env_f)(api_command_ptr->args[0].get_u32(),
                                                                                          api_command_ptr->args[1].get_u32(),
                                                                                          api_command_ptr->args[2].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D:
                    {
                        reinterpret_cast<PFNGLTEXIMAGE2DPROC>(OpenGL::g_cached_gl_tex_image_2D)(api_command_ptr->args[0].get_u32(),
                                                                                                api_command_ptr->args[1].get_i32(),
                                                                                                api_command_ptr->args[2].get_i32(),
                                                                                                api_command_ptr->args[3].get_i32(),
                                                                                                api_command_ptr->args[4].get_i32(),
                                                                                                api_command_ptr->args[5].get_i32(),
                                                                                                api_command_ptr->args[6].get_u32(),
                                                                                                api_command_ptr->args[7].get_u32(),
                                                                                                api_command_ptr->args[8].get_ptr() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERF:
                    {
                        reinterpret_cast<PFNGLTEXPARAMETERFPROC>(OpenGL::g_cached_gl_tex_parameterf)(api_command_ptr->args[0].get_u32 (),
                                                                                                     api_command_ptr->args[1].get_u32 (),
                                                                                                     api_command_ptr->args[2].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLTRANSLATEF:
                    {
                        reinterpret_cast<PFNGLTRANSLATEFPROC>(OpenGL::g_cached_gl_translate_f)(api_command_ptr->args[0].get_fp32(),
                                                                                               api_command_ptr->args[1].get_fp32(),
                                                                                               api_command_ptr->args[2].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLVERTEX2F:
                    {
                        const float original_x = api_command_ptr->args[0].get_fp32();
                        const float original_y = api_command_ptr->args[1].get_fp32();
                        float       x          = original_x;
                        float       y          = original_y;

                        if (is_status_bar_band_texture_bound)
                        {
                            continue;
                        }

                        if (is_ortho_enabled)
                        {
                            // We're likely rendering UI. Lerp the input coords so that they are retrofitted to our modified
                            // orthogonal projection matrix.
                            const auto eye_texture_resolution = m_vr_playback_ptr->get_eye_texture_resolution(in_left_eye);
                            const auto ortho_height           = ortho_x2y2.at(1) - ortho_x1y1.at(1);
                            const auto ortho_width            = ortho_x2y2.at(0) - ortho_x1y1.at(0);

                            x = static_cast<float>(ortho_x1y1.at(0)) + x / static_cast<float>(q1_hud_width)  * static_cast<float>(ortho_width);
                            y = static_cast<float>(ortho_x1y1.at(1)) + y / static_cast<float>(q1_hud_height) * static_cast<float>(ortho_height);

                            if (!is_console_texture_bound &&
                                !status_bar_rendered)
                            {
                                // Translate the status bar if we're dealing with one.
                                if (original_y >= q1_hud_height - q1_status_bar_height)
                                {
                                    y += m_settings_ptr->get_status_bar_y_offset();
                                }
                            }
                            else
                            {
                                if (!is_console_texture_bound &&
                                     status_bar_rendered)
                                {
                                    // This is the menu window (the one seen when you press ESC).
                                    y = static_cast<float>(ortho_height) * 0.5f + y / 480.0f * static_cast<float>(ortho_x2y2.at(1) - ortho_x1y1.at(1)) * 0.25f;
                                }
                                else
                                {
                                    // Handle console window separately.
                                    y += m_settings_ptr->get_console_window_y_offset();
                                }
                            }
                        }
                        reinterpret_cast<PFNGLVERTEX2FPROC>(OpenGL::g_cached_gl_vertex_2f)(x,
                                                                                           y);

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLVERTEX3F:
                    {
                        AI_ASSERT(!is_ortho_enabled);

                        reinterpret_cast<PFNGLVERTEX3FPROC>(OpenGL::g_cached_gl_vertex_3f)(api_command_ptr->args[0].get_fp32(),
                                                                                           api_command_ptr->args[1].get_fp32(),
                                                                                           api_command_ptr->args[2].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLVERTEX4F:
                    {
                        AI_ASSERT(!is_ortho_enabled);

                        reinterpret_cast<PFNGLVERTEX4FPROC>(OpenGL::g_cached_gl_vertex_4f)(api_command_ptr->args[0].get_fp32(),
                                                                                           api_command_ptr->args[1].get_fp32(),
                                                                                           api_command_ptr->args[2].get_fp32(),
                                                                                           api_command_ptr->args[3].get_fp32() );

                        break;
                    }

                    case APIInterceptor::APIFUNCTION_GL_GLVIEWPORT:
                    {
                        // LERP the specified viewport region to eye texture's resolution.
                        int32_t original_viewport_x1     = api_command_ptr->args[0].get_i32();
                        int32_t original_viewport_y1     = api_command_ptr->args[1].get_i32();
                        int32_t original_viewport_width  = api_command_ptr->args[2].get_i32();
                        int32_t original_viewport_height = api_command_ptr->args[3].get_i32();
                        int32_t original_viewport_x2     = original_viewport_x1 + original_viewport_width;
                        int32_t original_viewport_y2     = original_viewport_y1 + original_viewport_height;

                        float x1_norm     = static_cast<float>(original_viewport_x1)     / static_cast<float>(viewport_extents.at(0) );
                        float y1_norm     = static_cast<float>(original_viewport_y1)     / static_cast<float>(viewport_extents.at(1) );
                        float width_norm  = static_cast<float>(original_viewport_width)  / static_cast<float>(viewport_extents.at(0) );
                        float height_norm = static_cast<float>(original_viewport_height) / static_cast<float>(viewport_extents.at(1) );

                        int32_t new_viewport_x1     = static_cast<int32_t>(x1_norm     * static_cast<float>(viewport_extents.at(0) ));
                        int32_t new_viewport_y1     = static_cast<int32_t>(y1_norm     * static_cast<float>(viewport_extents.at(1) ));
                        int32_t new_viewport_width  = static_cast<int32_t>(width_norm  * static_cast<float>(viewport_extents.at(0) ));
                        int32_t new_viewport_height = static_cast<int32_t>(height_norm * static_cast<float>(viewport_extents.at(1) ));

                        reinterpret_cast<PFNGLVIEWPORTPROC>(OpenGL::g_cached_gl_viewport)(new_viewport_x1,
                                                                                          new_viewport_y1,
                                                                                          new_viewport_width,
                                                                                          new_viewport_height);

                        break;
                    }

                    default:
                    {
                        assert(false);
                    }
                }
            }
        }
    }

    /* Reset the matrices before we return.. */
    reinterpret_cast<PFNGLMATRIXMODEPROC>  (OpenGL::g_cached_gl_matrix_mode)  (GL_MODELVIEW);
    reinterpret_cast<PFNGLLOADIDENTITYPROC>(OpenGL::g_cached_gl_load_identity)();
    reinterpret_cast<PFNGLMATRIXMODEPROC>  (OpenGL::g_cached_gl_matrix_mode)  (GL_PROJECTION);
    reinterpret_cast<PFNGLLOADIDENTITYPROC>(OpenGL::g_cached_gl_load_identity)();
}