/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
 #if !defined(VR_PLAYER_TYPES_H)
 #define VR_PLAYER_TYPES_H

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include <vector>
#include <Windows.h>

#include "Common/types.h"

class Frame;
class FramePlayer;
class FrameInterceptor;
class PlaybackOpenXR;
class PlaybackOVR;
class PreviewWindow;
class PreviewWindowUI;
class Settings;
class SlabAllocator;
class VRPlayer;
class VRRenderer;

struct FrameState
{
    uint32_t alpha_func_func;
    float    alpha_func_ref;
    uint32_t blend_func_dfactor;
    uint32_t blend_func_sfactor;
    uint32_t bound_texture_2d_gl_id;
    uint32_t cull_face_mode;
    uint32_t depth_func;
    bool     depth_mask;
    uint32_t front_face_mode;
    uint32_t matrix_mode;
    uint32_t polygon_mode_back_mode;
    uint32_t polygon_mode_front_mode;
    uint32_t shade_model;
    uint32_t texture_env_mode;

    float  clear_color      [4];
    float  clear_depth;
    float  depth_range      [2];
    double modelview_matrix [16]; //< NOTE: This state is only updated prior to frame present.
    double projection_matrix[16]; //< NOTE: This state is only updated prior to frame present.

    int32_t viewport_extents[2];
    int32_t viewport_x1y1   [2];

    bool is_alpha_test_enabled;
    bool is_blend_enabled;
    bool is_cull_face_enabled;
    bool is_depth_test_enabled;
    bool is_scissor_test_enabled;
    bool is_texture_2d_enabled;

    FrameState();
};

class IVRPlayback
{
public:
    virtual ~IVRPlayback()
    {
        /* Stub */
    }

    virtual bool                    acquire_eye_texture                         (const bool&                    in_left_eye,
                                                                                 uint32_t*                      out_eye_color_texture_id_ptr)             = 0;
    virtual bool                    commit_eye_texture                          ()                                                                        = 0;
    virtual void                    deinit_for_bound_gl_context                 ()                                                                        = 0;
    virtual float                   get_current_pitch_angle                     ()                                                                  const = 0;
    virtual float                   get_current_yaw_angle                       ()                                                                  const = 0;
    virtual float                   get_eye_offset_x                            (const bool&                    in_left_eye,
                                                                                 const bool&                    in_multiply_by_user_setting)        const = 0;
    virtual std::array<uint32_t, 2> get_eye_texture_resolution                  (const bool&                    in_left_eye)                        const = 0;
    virtual uint32_t                get_preview_texture_gl_id                   ()                                                                  const = 0;
    virtual float                   get_tan_between_view_vec_and_bottom_fov_edge(const bool&                    in_left_eye)                        const = 0;
    virtual float                   get_tan_between_view_vec_and_top_fov_edge   (const bool&                    in_left_eye)                        const = 0;
    virtual bool                    present                                     ()                                                                        = 0;
    virtual bool                    setup_for_bound_gl_context                  (const std::array<uint32_t, 2>& in_preview_texture_extents_u32vec2)       = 0;
};

class IPreviewWindowCallback
{
public:
    virtual ~IPreviewWindowCallback()
    {
        /* Stub */
    }

    virtual void on_frame_available(const Frame* in_frame_ptr) const = 0;
};


typedef std::unique_ptr<FramePlayer>                            FramePlayerUniquePtr;
typedef std::unique_ptr<Frame>                                  FrameUniquePtr;
typedef std::unique_ptr<FrameInterceptor>                       FrameInterceptorUniquePtr;
typedef std::unique_ptr<PlaybackOpenXR>                         PlaybackOpenXRUniquePtr;
typedef std::unique_ptr<PlaybackOVR>                            PlaybackOVRUniquePtr;
typedef std::unique_ptr<PreviewWindow>                          PreviewWindowUniquePtr;
typedef std::unique_ptr<PreviewWindowUI>                        PreviewWindowUIUniquePtr;
typedef std::unique_ptr<Settings>                               SettingsUniquePtr;
typedef std::unique_ptr<uint8_t[], std::function<void(void*)> > SlabAllocationUniquePtr;
typedef std::unique_ptr<SlabAllocator>                          SlabAllocatorUniquePtr;
typedef std::unique_ptr<VRRenderer>                             VRRendererUniquePtr;

#endif /* VR_PLAYER_TYPES_H */