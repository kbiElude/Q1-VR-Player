/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_H)
#define VRPLAYER_H

#include "VRPlayer_types.h"

/* Forward decls */
class                             VRPlayer;
typedef std::unique_ptr<VRPlayer> VRPlayerUniquePtr;

class VRPlayer
{
public:
    /* Public constants */

    /* Tells how many times the preview texture should be smaller compared to original eye texture's extents */
    static const uint32_t EYE_TO_PREVIEW_TEXTURE_DIVISOR = 4;

    /* Public funcs */
    static VRPlayerUniquePtr create();

    const HWND& get_q1_hwnd() const
    {
        return m_q1_hwnd;
    }

    ~VRPlayer();

private:
    /* Private funcs */
    VRPlayer();

    bool init              ();
    void reposition_windows();

    // Q1 API call interceptors -->
    static void on_q1_wgldeletecontext(APIInterceptor::APIFunction                in_api_func,
                                       uint32_t                                   in_n_args,
                                       const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                       void*                                      in_user_arg_ptr,
                                       bool*                                      out_should_pass_through_ptr);
    static void on_q1_wglmakecurrent  (APIInterceptor::APIFunction                in_api_func,
                                       uint32_t                                   in_n_args,
                                       const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                       void*                                      in_user_arg_ptr,
                                       bool*                                      out_should_pass_through_ptr);
    // <--

    /* Private vars */
    HWND m_q1_hwnd;

    FrameInterceptorUniquePtr m_frame_interceptor_ptr;
    FramePlayerUniquePtr      m_frame_player_ptr;
    PreviewWindowUniquePtr    m_preview_window_ptr;
    SettingsUniquePtr         m_settings_ptr;
    SlabAllocatorUniquePtr    m_slab_allocator_ptr;
    VRPlaybackUniquePtr       m_vr_playback_ptr;
    VRRendererUniquePtr       m_vr_renderer_ptr;
};

#endif /* VRPLAYER_H */