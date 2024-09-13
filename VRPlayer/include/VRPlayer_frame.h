/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VRPLAYER_FRAME_H)
#define VRPLAYER_FRAME_H

#include "APIInterceptor/include/Common/types.h"
#include "VRPlayer_types.h"

constexpr uint32_t N_MAX_FRAME_API_COMMAND_ARGS = 9;

struct FrameAPICommand
{
    APIInterceptor::APIFunction         api_func;
    APIInterceptor::APIFunctionArgument args[N_MAX_FRAME_API_COMMAND_ARGS];
    uint32_t                            n_args;

    FrameAPICommand()
        :api_func(APIInterceptor::APIFUNCTION_UNKNOWN),
         n_args(0)
    {
        /* Stub */
    }
};


class Frame
{
public:
    /* Public funcs */
    ~Frame();

    static FrameUniquePtr create(SlabAllocator* in_slab_allocator_ptr);

    uint32_t               get_n_api_commands  ()                                 const;
    const FrameAPICommand* get_api_command_ptr (const uint32_t& in_n_api_command) const;

    const FrameState* get_start_state_ptr() const
    {
        return &m_start_state;
    }

    void record_api_call(const APIInterceptor::APIFunction&         in_api_func,
                         const uint32_t&                            in_n_args,
                         const APIInterceptor::APIFunctionArgument* in_args_ptr);
    void reset          ();

    void set_start_state(const FrameState& in_start_state)
    {
        m_start_state = in_start_state;
    }

private:
    /* Private funcs */
    Frame();

    bool init(SlabAllocator* in_slab_allocator_ptr);

    /* Private vars */
    uint32_t                m_n_api_commands_used;
    SlabAllocationUniquePtr m_slab_allocation_ptr;
    uint32_t                m_slab_allocation_size;

    FrameState m_start_state;
};

#endif /* VRPLAYER_FRAME_H */