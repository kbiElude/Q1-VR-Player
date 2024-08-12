/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "OpenGL/globals.h"
#include "common_defines.inl"
#include "VRPlayer_types.h"

FrameState::FrameState()
    :alpha_func_func        (GL_ALWAYS),
     alpha_func_ref         (0.0f),
     blend_func_dfactor     (GL_ZERO),
     blend_func_sfactor     (GL_ONE),
     bound_texture_2d_gl_id (0),
     clear_color            {},
     clear_depth            (1.0f),
     cull_face_mode         (GL_BACK),
     depth_func             (GL_LESS),
     depth_mask             (true),
     depth_range            {0.0f, 1.0f},
     front_face_mode        (GL_CCW),
     matrix_mode            (GL_MODELVIEW),
     is_alpha_test_enabled  (false),
     is_blend_enabled       (false),
     is_cull_face_enabled   (false),
     is_depth_test_enabled  (false),
     is_scissor_test_enabled(false),
     is_texture_2d_enabled  (false),
     polygon_mode_back_mode (GL_FILL),
     polygon_mode_front_mode(GL_FILL),
     shade_model            (GL_SMOOTH),
     texture_env_mode       (GL_MODULATE),
     viewport_x1y1          {}
{
    modelview_matrix[0]  = 1; modelview_matrix[1]  = 0; modelview_matrix[2]  = 0; modelview_matrix[3]  = 0;
    modelview_matrix[4]  = 0; modelview_matrix[5]  = 1; modelview_matrix[6]  = 0; modelview_matrix[7]  = 0;
    modelview_matrix[8]  = 0; modelview_matrix[9]  = 0; modelview_matrix[10] = 1; modelview_matrix[11] = 0;
    modelview_matrix[12] = 0; modelview_matrix[13] = 0; modelview_matrix[14] = 0; modelview_matrix[15] = 1;

    memcpy(projection_matrix,
           modelview_matrix,
           sizeof(double) * 16);

    viewport_extents[0] = Q1_NATIVE_RENDERING_WIDTH;
    viewport_extents[1] = Q1_NATIVE_RENDERING_HEIGHT;
}