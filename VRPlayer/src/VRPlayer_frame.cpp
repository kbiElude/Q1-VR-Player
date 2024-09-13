/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include <cassert>
#include <functional>
#include "OpenGL/globals.h"
#include "VRPlayer_frame.h"
#include "VRPlayer_slab_allocator.h"

Frame::Frame()
    :m_n_api_commands_used           (0),
     m_n_bind_console_texture_command(UINT32_MAX)
{
    /* Stub */
}

Frame::~Frame()
{
    m_slab_allocation_ptr.reset();
}

FrameUniquePtr Frame::create(SlabAllocator* in_slab_allocator_ptr)
{
    FrameUniquePtr result_ptr(new Frame() );

    assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init(in_slab_allocator_ptr) )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

uint32_t Frame::get_n_api_commands() const
{
    return m_n_api_commands_used;
}

const FrameAPICommand* Frame::get_api_command_ptr(const uint32_t& in_n_api_command) const
{
    AI_ASSERT(sizeof(FrameAPICommand) * in_n_api_command < m_slab_allocation_size);

    return reinterpret_cast<const FrameAPICommand*>(m_slab_allocation_ptr.get() + sizeof(FrameAPICommand) * in_n_api_command);
}

bool Frame::init(SlabAllocator* in_slab_allocator_ptr)
{
    bool result = true;

    m_slab_allocation_ptr = in_slab_allocator_ptr->allocate(SlabAllocator::MAX_SLAB_SIZE);

    AI_ASSERT(m_slab_allocation_ptr != nullptr);
    if (m_slab_allocation_ptr == nullptr)
    {
        result = false;

        goto end;
    }

end:
    return result;
}

void Frame::record_api_call(const APIInterceptor::APIFunction&         in_api_func,
                            const uint32_t&                            in_n_args,
                            const APIInterceptor::APIFunctionArgument* in_args_ptr)
{
    static const uint32_t console_background_texture_id = 2;

    FrameAPICommand* new_api_command_ptr = reinterpret_cast<FrameAPICommand*>(m_slab_allocation_ptr.get() + sizeof(FrameAPICommand) * m_n_api_commands_used);

    AI_ASSERT(in_n_args <= sizeof(new_api_command_ptr->args) / sizeof(new_api_command_ptr->args[0]) );

    new_api_command_ptr->api_func = in_api_func;
    new_api_command_ptr->n_args   = in_n_args;

    memcpy(new_api_command_ptr->args,
           in_args_ptr,
           sizeof(APIInterceptor::APIFunctionArgument) * in_n_args);

    if (in_api_func              == APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE &&
        in_args_ptr[1].get_u32() == console_background_texture_id)
    {
        m_n_bind_console_texture_command = m_n_api_commands_used;
    }

    m_n_api_commands_used++;
}

void Frame::reset()
{
    m_n_api_commands_used = 0;
}

