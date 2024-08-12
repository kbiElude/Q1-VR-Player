/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_SLAB_ALLOCATOR_H)
#define VR_PLAYER_SLAB_ALLOCATOR_H

#include "VRPlayer_types.h"

/* The simplest allocator ever.
 *
 * Returns preallocated chunks of memory that fit the caller's requirement. No support for
 * carving subchunks at this point (not needed, so KISS).
 *
 * This implementation does not provide any synchronization guarantees. If used in multithreaded
 * context, the user MUST take care of serializing the calls.
 */
class SlabAllocator
{
public:
    /* Public consts */
    static const uint32_t MAX_SLAB_SIZE        = 32 * 1024768; /* 32 mb */
    static const uint32_t N_PREALLOCATED_SLABS = 2;

    /* Public funcs */

    static SlabAllocatorUniquePtr create();

    ~SlabAllocator();

    /* @param in_size Must not be larger than MAX_SLAB_SIZE. */
    SlabAllocationUniquePtr allocate(const uint32_t& in_size,
                                     const bool&     in_force_alloc_new_chunk = false);

private:
    /* Private funcs */
    SlabAllocator();

    bool init                 ();
    void on_alloc_out_of_scope(void* in_alloc_mem_ptr);

    /* Private vars */
    bool                                 m_being_destroyed;
    std::vector<SlabAllocationUniquePtr> m_unused_slab_alloc_ptr_vec;
    std::vector<void*>                   m_used_slab_alloc_ptr_vec;
};

#endif /* VR_PLAYER_SLAB_ALLOCATOR_H */