/*
 * Copyright 2022 The Hafnium Authors.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/BSD-3-Clause.
 */

/**
 * The maximum number of memory sharing handles which may be active at once. A
 * DONATE handle is active from when it is sent to when it is retrieved; a SHARE
 * or LEND handle is active from when it is sent to when it is reclaimed.
 */
#define MAX_MEM_SHARES 100

/**
 * The maximum number of fragments into which a memory sharing message may be
 * broken.
 */
#define MAX_FRAGMENTS 20

static_assert(sizeof(struct ffa_memory_region_constituent) % 16 == 0,
	      "struct ffa_memory_region_constituent must be a multiple of 16 "
	      "bytes long.");
static_assert(sizeof(struct ffa_composite_memory_region) % 16 == 0,
	      "struct ffa_composite_memory_region must be a multiple of 16 "
	      "bytes long.");
static_assert(sizeof(struct ffa_memory_region_attributes) == 4,
	      "struct ffa_memory_region_attributes must be 4 bytes long.");
static_assert(sizeof(struct ffa_memory_access) % 16 == 0,
	      "struct ffa_memory_access must be a multiple of 16 bytes long.");
static_assert(sizeof(struct ffa_memory_region) % 16 == 0,
	      "struct ffa_memory_region must be a multiple of 16 bytes long.");
static_assert(sizeof(struct ffa_mem_relinquish) % 16 == 0,
	      "struct ffa_mem_relinquish must be a multiple of 16 "
	      "bytes long.");

struct ffa_memory_share_state {
	/**
	 * The memory region being shared, or NULL if this share state is
	 * unallocated.
	 */
	struct ffa_memory_region *memory_region;

	struct ffa_memory_region_constituent *fragments[MAX_FRAGMENTS];

	/** The number of constituents in each fragment. */
	uint32_t fragment_constituent_counts[MAX_FRAGMENTS];

	/**
	 * The number of valid elements in the `fragments` and
	 * `fragment_constituent_counts` arrays.
	 */
	uint32_t fragment_count;

	/**
	 * The FF-A function used for sharing the memory. Must be one of
	 * FFA_MEM_DONATE_32, FFA_MEM_LEND_32 or FFA_MEM_SHARE_32 if the
	 * share state is allocated, or 0.
	 */
	uint32_t share_func;

	/**
	 * The sender's original mode before invoking the FF-A function for
	 * sharing the memory.
	 * This is used to reset the original configuration when sender invokes
	 * FFA_MEM_RECLAIM_32.
	 */
	uint32_t sender_orig_mode;

	/**
	 * True if all the fragments of this sharing request have been sent and
	 * Hafnium has updated the sender page table accordingly.
	 */
	bool sending_complete;

	/**
	 * How many fragments of the memory region each recipient has retrieved
	 * so far. The order of this array matches the order of the endpoint
	 * memory access descriptors in the memory region descriptor. Any
	 * entries beyond the receiver_count will always be 0.
	 */
	uint32_t retrieved_fragment_count[MAX_MEM_SHARE_RECIPIENTS];

	/**
	 * Field for the SPMC to keep track of how many fragments of the memory
	 * region the hypervisor has managed to retrieve, using a
	 * `hypervisor retrieve request`, as defined by FF-A v1.1 EAC0
	 * specification.
	 */
	uint32_t hypervisor_fragment_count;
};

/**
 * Encapsulates the set of share states while the `share_states_lock` is held.
 */
struct share_states_locked {
	struct ffa_memory_share_state *share_states;
};

bool allocate_share_state(struct share_states_locked share_states,
			  uint32_t share_func,
			  struct ffa_memory_region *memory_region,
			  uint32_t fragment_length, ffa_memory_handle_t handle,
			  struct ffa_memory_share_state **share_state_ret);
struct share_states_locked share_states_lock(void);
void share_states_unlock(struct share_states_locked *share_states);
void dump_share_states(void);

/**
 * Return the offset to the first constituent within the
 * `ffa_composite_memory_region` for the given receiver from an
 * `ffa_memory_region`. The caller must check that the receiver_index is within
 * bounds, and that it has a composite memory region offset.
 */
static inline uint32_t ffa_composite_constituent_offset(
	struct ffa_memory_region *memory_region, uint32_t receiver_index)
{
	CHECK(receiver_index < memory_region->receiver_count);
	CHECK(memory_region->receivers[receiver_index]
		      .composite_memory_region_offset != 0);

	return memory_region->receivers[receiver_index]
		       .composite_memory_region_offset +
	       sizeof(struct ffa_composite_memory_region);
}

struct ffa_value ffa_memory_send_validate(
	struct vm_locked from_locked, struct ffa_memory_region *memory_region,
	uint32_t memory_share_length, uint32_t fragment_length,
	uint32_t share_func);
struct ffa_value ffa_send_check_update(
	struct vm_locked from_locked,
	struct ffa_memory_region_constituent **fragments,
	uint32_t *fragment_constituent_counts, uint32_t fragment_count,
	uint32_t share_func, struct ffa_memory_access *receivers,
	uint32_t receivers_count, struct mpool *page_pool, bool clear,
	uint32_t *orig_from_mode_ret);
bool ffa_region_group_identity_map(
	struct vm_locked vm_locked,
	struct ffa_memory_region_constituent **fragments,
	const uint32_t *fragment_constituent_counts, uint32_t fragment_count,
	uint32_t mode, struct mpool *ppool, bool commit);
struct ffa_value ffa_retrieve_check_transition(
	struct vm_locked to, uint32_t share_func,
	struct ffa_memory_region_constituent **fragments,
	uint32_t *fragment_constituent_counts, uint32_t fragment_count,
	uint32_t memory_to_attributes, uint32_t *to_mode);