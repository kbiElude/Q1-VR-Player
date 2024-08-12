/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#include "VRPlayer_slab_allocator.h"


SlabAllocator::SlabAllocator()
    :m_being_destroyed(false)
{
    // ..
}

SlabAllocator::~SlabAllocator()
{
    // You do not want this assert to fail. Ever.
    AI_ASSERT(m_used_slab_alloc_ptr_vec.size() == 0);

    m_being_destroyed = true;

    m_unused_slab_alloc_ptr_vec.clear();
    m_used_slab_alloc_ptr_vec.clear  ();
}

SlabAllocationUniquePtr SlabAllocator::allocate(const uint32_t& in_size,
                                                const bool&     in_force_alloc_new_chunk)
{
    SlabAllocationUniquePtr result_ptr(nullptr,
                                       std::bind(&SlabAllocator::on_alloc_out_of_scope,
                                                  this,
                                                  std::placeholders::_1) );

    if (m_unused_slab_alloc_ptr_vec.size() == 0    ||
        in_force_alloc_new_chunk           == true)
    {
        result_ptr.reset(
            new uint8_t[MAX_SLAB_SIZE]
        );
    }
    else
    {
        result_ptr = std::move(m_unused_slab_alloc_ptr_vec.back() );

        m_unused_slab_alloc_ptr_vec.pop_back();
    }

    AI_ASSERT(result_ptr != nullptr);

    m_used_slab_alloc_ptr_vec.emplace_back(result_ptr.get() );
    return result_ptr;
}

SlabAllocatorUniquePtr SlabAllocator::create()
{
    SlabAllocatorUniquePtr result_ptr(new SlabAllocator() );

    AI_ASSERT(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool SlabAllocator::init()
{
    bool result = true;

    for (uint32_t n_slab = 0;
                  n_slab < N_PREALLOCATED_SLABS;
                ++n_slab)
    {
        if (allocate(MAX_SLAB_SIZE,
                     true /* in_force_alloc_new_chunk */) == nullptr)
        {
            AI_ASSERT_FAIL();

            result = false;
            goto end;
        }
    }

end:
    return result;
}

void SlabAllocator::on_alloc_out_of_scope(void* in_alloc_mem_ptr)
{
    {
        auto used_alloc_mem_ptr_iterator = std::find(m_used_slab_alloc_ptr_vec.begin(),
                                                     m_used_slab_alloc_ptr_vec.end  (),
                                                     in_alloc_mem_ptr);

        if (!m_being_destroyed)
        {
            AI_ASSERT(used_alloc_mem_ptr_iterator != m_used_slab_alloc_ptr_vec.end() );
        }

        if (used_alloc_mem_ptr_iterator != m_used_slab_alloc_ptr_vec.end())
        {
            m_used_slab_alloc_ptr_vec.erase(used_alloc_mem_ptr_iterator);
        }
    }

    if (!m_being_destroyed)
    {
        SlabAllocationUniquePtr wrapped_ptr(static_cast<uint8_t*>(in_alloc_mem_ptr),
                                            std::bind(&SlabAllocator::on_alloc_out_of_scope,
                                                      this,
                                                      std::placeholders::_1) );

        m_unused_slab_alloc_ptr_vec.emplace_back(
            std::move(wrapped_ptr)
        );
    }
    else
    {
        delete [] in_alloc_mem_ptr;
    }
}