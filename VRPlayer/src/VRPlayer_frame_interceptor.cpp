/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "Common/callbacks.h"
#include "OpenGL/globals.h"
#include "VRPlayer.h"
#include "VRPlayer_frame.h"
#include "VRPlayer_frame_interceptor.h"
#include "VRPlayer_slab_allocator.h"

FrameInterceptor::FrameInterceptor(const IPreviewWindowCallback* in_preview_window_callback_ptr)
    :m_frame_misc_data_slab_start_offset(0),
     m_preview_window_callback_ptr      (in_preview_window_callback_ptr)
{
    // ..
}

FrameInterceptor::~FrameInterceptor()
{
    m_frame_ptr.reset();
}

FrameInterceptorUniquePtr FrameInterceptor::create(SlabAllocator*                in_slab_allocator_ptr,
                                                   const IPreviewWindowCallback* in_preview_window_callback_ptr)
{
    FrameInterceptorUniquePtr result_ptr(new FrameInterceptor(in_preview_window_callback_ptr) );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init(in_slab_allocator_ptr) )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool FrameInterceptor::init(SlabAllocator* in_slab_allocator_ptr)
{
    bool result = false;

    /* Allocate space for frame data storage */
    m_frame_misc_data_slab_alloc_ptr = in_slab_allocator_ptr->allocate(SlabAllocator::MAX_SLAB_SIZE);
    m_frame_ptr                      = Frame::create                  (in_slab_allocator_ptr);

    AI_ASSERT(m_frame_misc_data_slab_alloc_ptr != nullptr);
    AI_ASSERT(m_frame_ptr                      != nullptr);

    if (m_frame_misc_data_slab_alloc_ptr == nullptr ||
        m_frame_ptr                      == nullptr)
    {
        goto end;
    }

    /* Initialize callback handlers for all GL entrypoints used by Q1. */
    for (uint32_t current_api_func =  static_cast<uint32_t>(APIInterceptor::APIFUNCTION_GL_FIRST);
                  current_api_func <= static_cast<uint32_t>(APIInterceptor::APIFUNCTION_GL_LAST);
                ++current_api_func)
    {
        APIInterceptor::register_for_callback(static_cast<APIInterceptor::APIFunction>(current_api_func),
                                              FrameInterceptor::on_api_func_callback,
                                              this);
    }

    /* We also need to know when we're done with a snapshot. */
    APIInterceptor::register_for_callback(APIInterceptor::APIFUNCTION_GDI32_SWAPBUFFERS,
                                          FrameInterceptor::on_api_func_callback,
                                          this);

    result = true;
end:
    return result;
}

void FrameInterceptor::on_api_func_callback(APIInterceptor::APIFunction                in_api_func,
                                            uint32_t                                   in_n_args,
                                            const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                            void*                                      in_user_arg_ptr,
                                            bool*                                      out_should_pass_through_ptr)
{
    APIInterceptor::APIFunctionArgument args[N_MAX_FRAME_API_COMMAND_ARGS];
    bool                                should_pass_through                = true;
    FrameInterceptor*                   this_ptr                           = reinterpret_cast<FrameInterceptor*>(in_user_arg_ptr);

    /* This function is called from within application's thread.
     *
     * We do not immediately play back the command in our own rendering thread which lives in a separate thread,
     * because that would require expensive inter-thread synchronization. Instead, we stash commands as the app
     * continues to fire them. At frame present time, we block app's thread, revive the preview window's thread,
     * and ask it to process the frame (which means re-rendering the frame from two different eye locations to
     * dedicated 2D textures)
     *
     * There's a few bad guys that could hurt us if we don't pay attention:
     *
     * 1) Some API calls pass pointers. These pointers, in all likelihood, are going to be invalid or are going
     *    to hold incorrect data at playback time. In some cases, we can convert such calls to their non-ptr
     *    equivalents, but in others we need to cache the data locally.
     * 2) While we *could* technically unlock the game's thread after our rendering thread wakes up, this could
     *    cause trouble if the next frame's data arrives before our rendering thread completes processing. We
     *    could introduce hitches to the application's thread if we needed to wait for our thread to complete before
     *    we returned from this function. To save ourselves the headache, we just block. This shouldn't be a problem
     *    most of the time. (but we might need to revisit this in the future)
     * 3) In certain cases, apps submit data via pointers (see special handling of glTexImage2D() below).
     *    APIFunctionArgument structure does not support varying-sized data storage, so we need to store it in
     *    a preallocated memory chunk. Being able to handle multiple frames in flight would significantly complicate
     *    the code - yet another way not to do it unless we need to in the future
     * 4) In our rendering thread, we use VR libraries that generate their own texture IDs. This happens before the first
     *    frame is played back. Games like Q1 do NOT use glGenTextures() but assume whole texture namespace is available,
     *    which means we need to remap game's texture IDs to IDs used in our rendering thread. We do not need to worry
     *    about this here.
     *
     */
    AI_ASSERT(in_api_func != APIInterceptor::APIFUNCTION_GL_GLGENTEXTURES);
    AI_ASSERT(in_n_args   <= N_MAX_FRAME_API_COMMAND_ARGS);

    if (in_api_func != APIInterceptor::APIFUNCTION_GDI32_SWAPBUFFERS)
    {
        /* Is this an API function call that uses pointers as arguments? */
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCOLOR3UBV)
        {
            /* Convert to non-ptr representation.. */
            const auto color_data_ptr = reinterpret_cast<const unsigned char*>(in_args_ptr[0].get_u8_ptr() );

            args[0] = APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[0]);
            args[1] = APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[1]);
            args[2] = APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[2]);

            in_api_func = APIInterceptor::APIFUNCTION_GL_GLCOLOR3UB;
            in_args_ptr = args;
            in_n_args   = 3;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCOLOR4FV)
        {
            /* Convert to non-ptr representation.. */
            const auto color_data_ptr = in_args_ptr[0].get_fp32_ptr();

            args[0] = APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[0]);
            args[1] = APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[1]);
            args[2] = APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[2]);
            args[3] = APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[3]);

            in_api_func = APIInterceptor::APIFUNCTION_GL_GLCOLOR4F;
            in_args_ptr = args;
            in_n_args   = 4;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D)
        {
            int32_t     call_arg_width         = in_args_ptr[3].get_i32();
            int32_t     call_arg_height        = in_args_ptr[4].get_i32();
            uint32_t    call_arg_format        = in_args_ptr[6].get_u32();
            uint32_t    call_arg_type          = in_args_ptr[7].get_u32();
            const void* call_arg_pixels_ptr    = in_args_ptr[8].get_ptr();
            uint8_t*    local_data_storage_ptr = nullptr;

            // NOTE: Q1 does not adjust UNPACK_ALIGNMENT, whose default value is 4.
            //       This would break if type was not unsigned_byte!
            const auto call_arg_width_rounded_up_to_4 = (call_arg_width + 3) & ~3;

            AI_ASSERT(in_n_args       == 9);
            AI_ASSERT(call_arg_format == GL_LUMINANCE      || call_arg_format  == GL_RGBA);
            AI_ASSERT(call_arg_type   == GL_UNSIGNED_BYTE);

            AI_ASSERT( (call_arg_width_rounded_up_to_4 % 4) == 0);

            {
                const auto n_components             = (call_arg_format == GL_RGBA) ? 4u
                                                                                   : 1u;
                const auto n_bytes_under_pixels_ptr = call_arg_width_rounded_up_to_4 * call_arg_height * n_components;

                AI_ASSERT(this_ptr->m_frame_misc_data_slab_start_offset + n_bytes_under_pixels_ptr <= SlabAllocator::MAX_SLAB_SIZE);

                local_data_storage_ptr = this_ptr->m_frame_misc_data_slab_alloc_ptr.get() + this_ptr->m_frame_misc_data_slab_start_offset;

                memcpy(local_data_storage_ptr,
                       call_arg_pixels_ptr,
                       n_bytes_under_pixels_ptr);

                this_ptr->m_frame_misc_data_slab_start_offset += n_bytes_under_pixels_ptr;
            }

            memcpy(args,
                   in_args_ptr,
                   sizeof(APIInterceptor::APIFunctionArgument) * in_n_args);

            args[8]     = APIInterceptor::APIFunctionArgument::create_void_ptr(local_data_storage_ptr);
            in_args_ptr = args;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLVERTEX3FV)
        {
            /* Convert to non-ptr representation.. */
            const auto vertex_data_ptr = in_args_ptr[0].get_fp32_ptr();

            args[0] = APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[0]);
            args[1] = APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[1]);
            args[2] = APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[2]);

            in_api_func = APIInterceptor::APIFUNCTION_GL_GLVERTEX3F;
            in_args_ptr = args;
            in_n_args   = 3;
        }

        // Cache context state..
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLALPHAFUNC)
        {
            this_ptr->m_frame_current_state.alpha_func_func = in_args_ptr[0].get_u32 ();
            this_ptr->m_frame_current_state.alpha_func_ref  = in_args_ptr[1].get_fp32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE)
        {
            AI_ASSERT(in_args_ptr[0].get_u32() == GL_TEXTURE_2D);

            this_ptr->m_frame_current_state.bound_texture_2d_gl_id = in_args_ptr[1].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBLENDFUNC)
        {
            this_ptr->m_frame_current_state.blend_func_sfactor = in_args_ptr[0].get_u32();
            this_ptr->m_frame_current_state.blend_func_dfactor = in_args_ptr[1].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCLEARCOLOR)
        {
            this_ptr->m_frame_current_state.clear_color[0] = in_args_ptr[0].get_fp32();
            this_ptr->m_frame_current_state.clear_color[1] = in_args_ptr[1].get_fp32();
            this_ptr->m_frame_current_state.clear_color[2] = in_args_ptr[2].get_fp32();
            this_ptr->m_frame_current_state.clear_color[3] = in_args_ptr[3].get_fp32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCLEARDEPTH)
        {
            this_ptr->m_frame_current_state.clear_depth = in_args_ptr[0].get_fp32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCULLFACE)
        {
            this_ptr->m_frame_current_state.cull_face_mode = in_args_ptr[0].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHFUNC)
        {
            this_ptr->m_frame_current_state.depth_func = in_args_ptr[0].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHMASK)
        {
            this_ptr->m_frame_current_state.depth_mask = in_args_ptr[0].get_u32() == 1;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHRANGE)
        {
            this_ptr->m_frame_current_state.depth_range[0] = static_cast<float>(in_args_ptr[0].get_fp64() );
            this_ptr->m_frame_current_state.depth_range[1] = static_cast<float>(in_args_ptr[1].get_fp64() );
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDISABLE ||
            in_api_func == APIInterceptor::APIFUNCTION_GL_GLENABLE)
        {
            const auto new_state_value = (in_api_func == APIInterceptor::APIFUNCTION_GL_GLENABLE);

            switch (in_args_ptr[0].get_u32())
            {
                case GL_ALPHA_TEST:   this_ptr->m_frame_current_state.is_alpha_test_enabled   = new_state_value; break;
                case GL_BLEND:        this_ptr->m_frame_current_state.is_blend_enabled        = new_state_value; break;
                case GL_CULL_FACE:    this_ptr->m_frame_current_state.is_cull_face_enabled    = new_state_value; break;
                case GL_DEPTH_TEST:   this_ptr->m_frame_current_state.is_depth_test_enabled   = new_state_value; break;
                case GL_SCISSOR_TEST: this_ptr->m_frame_current_state.is_scissor_test_enabled = new_state_value; break;
                case GL_TEXTURE_2D:   this_ptr->m_frame_current_state.is_texture_2d_enabled   = new_state_value; break;

                default:
                {
                    AI_ASSERT_FAIL();
                }
            }
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLFRONTFACE)
        {
            this_ptr->m_frame_current_state.front_face_mode = in_args_ptr[0].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLMATRIXMODE)
        {
            this_ptr->m_frame_current_state.matrix_mode = in_args_ptr[0].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLPOLYGONMODE)
        {
            const auto face = in_args_ptr[0].get_u32();
            const auto mode = in_args_ptr[1].get_u32();

            if (face == GL_BACK)
            {
                this_ptr->m_frame_current_state.polygon_mode_back_mode = mode;
            }
            else
            if (face == GL_FRONT)
            {
                this_ptr->m_frame_current_state.polygon_mode_front_mode = mode;
            }
            else
            {
                AI_ASSERT(face == GL_FRONT_AND_BACK);

                this_ptr->m_frame_current_state.polygon_mode_back_mode  = mode;
                this_ptr->m_frame_current_state.polygon_mode_front_mode = mode;
            }
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLSHADEMODEL)
        {
            this_ptr->m_frame_current_state.shade_model = in_args_ptr[0].get_u32();
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXENVF)
        {
            if (in_args_ptr[0].get_u32() == GL_TEXTURE_ENV      &&
                in_args_ptr[1].get_u32() == GL_TEXTURE_ENV_MODE)
            {
                this_ptr->m_frame_current_state.texture_env_mode = static_cast<uint32_t>(in_args_ptr[2].get_fp32() );
            }
            else
            {
                AI_ASSERT_FAIL();
            }
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLVIEWPORT)
        {
            this_ptr->m_frame_current_state.viewport_x1y1   [0] = in_args_ptr[0].get_i32();
            this_ptr->m_frame_current_state.viewport_x1y1   [1] = in_args_ptr[1].get_i32();
            this_ptr->m_frame_current_state.viewport_extents[0] = in_args_ptr[2].get_i32();
            this_ptr->m_frame_current_state.viewport_extents[1] = in_args_ptr[3].get_i32();
        }

        this_ptr->m_frame_ptr->record_api_call(in_api_func,
                                               in_n_args,
                                               in_args_ptr);
    }
    else
    {
        // Replay the frame for two eye locations.
        this_ptr->m_preview_window_callback_ptr->on_frame_available(this_ptr->m_frame_ptr.get() );

        /* Prepare for the next frame. */
        this_ptr->m_frame_misc_data_slab_start_offset = 0;

        this_ptr->m_frame_ptr->reset          ();
        this_ptr->m_frame_ptr->set_start_state(this_ptr->m_frame_current_state);

        // Cache modelview & projection matrices.
        reinterpret_cast<PFNGLGETDOUBLEVPROC>(OpenGL::g_cached_gl_get_doublev)(GL_MODELVIEW_MATRIX,
                                                                               this_ptr->m_frame_current_state.modelview_matrix);
        reinterpret_cast<PFNGLGETDOUBLEVPROC>(OpenGL::g_cached_gl_get_doublev)(GL_PROJECTION_MATRIX,
                                                                               this_ptr->m_frame_current_state.projection_matrix);

        // Do NOT swap buffers! We don't need to show anything in the game window.
        should_pass_through = false;
    }

    *out_should_pass_through_ptr = should_pass_through;
}
