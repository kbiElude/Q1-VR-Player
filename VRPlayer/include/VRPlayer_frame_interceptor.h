/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_FRAME_INTERCEPTOR_H)
#define VR_PLAYER_FRAME_INTERCEPTOR_H

#include "VRPlayer_types.h"


class FrameInterceptor
{
public:
    /* Public funcs */
    static FrameInterceptorUniquePtr create(SlabAllocator*                in_slab_allocator_ptr,
                                            const IPreviewWindowCallback* in_preview_window_callback_ptr);

    ~FrameInterceptor();

private:
    /* Private funcs */
    FrameInterceptor(const IPreviewWindowCallback* in_preview_window_callback_ptr);

    bool init(SlabAllocator* in_slab_allocator_ptr);

    static void on_api_func_callback(APIInterceptor::APIFunction                in_api_func,
                                     uint32_t                                   in_n_args,
                                     const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                     void*                                      in_user_arg_ptr,
                                     bool*                                      out_should_pass_through_ptr);

    /* Private vars */
    FrameState              m_frame_current_state;
    FrameUniquePtr          m_frame_ptr;
    SlabAllocationUniquePtr m_frame_misc_data_slab_alloc_ptr;
    uint32_t                m_frame_misc_data_slab_start_offset;

    const IPreviewWindowCallback* const m_preview_window_callback_ptr;
};

#endif /* VR_PLAYER_FRAME_INTERCEPTOR_H */