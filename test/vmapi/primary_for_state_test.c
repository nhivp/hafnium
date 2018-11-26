/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdalign.h>
#include <stdint.h>

#include "hf/arch/vm/power_mgmt.h"

#include "hf/mm.h"
#include "hf/std.h"

#include "vmapi/hf/call.h"

#include "hftest.h"

static alignas(PAGE_SIZE) uint8_t send_page[PAGE_SIZE];
static alignas(PAGE_SIZE) uint8_t recv_page[PAGE_SIZE];
static_assert(sizeof(send_page) == PAGE_SIZE, "Send page is not a page.");
static_assert(sizeof(recv_page) == PAGE_SIZE, "Recv page is not a page.");

static hf_ipaddr_t send_page_addr = (hf_ipaddr_t)send_page;
static hf_ipaddr_t recv_page_addr = (hf_ipaddr_t)recv_page;

#define STATE_VM_ID 1

/**
 * Iterates trying to run vCPU of the secondary VM. Returns when a message
 * of non-zero length is received.
 */
bool run_loop(void)
{
	struct hf_vcpu_run_return run_res;
	bool ok = false;

	for (;;) {
		/* Run until it manages to schedule vCPU on this CPU. */
		do {
			run_res = hf_vcpu_run(STATE_VM_ID, 0);
		} while (run_res.code == HF_VCPU_RUN_WAIT_FOR_INTERRUPT);

		/* Break out if we received a message with non-zero length. */
		if (run_res.code == HF_VCPU_RUN_MESSAGE &&
		    run_res.message.size != 0) {
			break;
		}

		/* Clear mailbox so that next message can be received. */
		hf_mailbox_clear();
	}

	/* Copies the contents of the received boolean to the return value. */
	if (run_res.message.size == sizeof(ok)) {
		memcpy(&ok, recv_page, sizeof(ok));
	}

	hf_mailbox_clear();

	return ok;
}

/**
 * This is the entry point of the additional primary VM vCPU. It just calls
 * the run loop so that two cpus compete for the chance to run a secondary VM.
 */
static void vm_cpu_entry(uintptr_t arg)
{
	(void)arg;
	run_loop();
}

/**
 * This test tries to run the same secondary vCPU from two different physical
 * CPUs concurrently. The vCPU checks that the state is ok while it bounces
 * between the physical CPUs.
 */
TEST(vcpu_state, concurrent_save_restore)
{
	alignas(4096) static char stack[4096];

	EXPECT_EQ(hf_vm_configure(send_page_addr, recv_page_addr), 0);

	/* Start second vCPU. */
	EXPECT_EQ(cpu_start(1, stack, sizeof(stack), vm_cpu_entry, 0), true);

	/* Run on a loop until the secondary VM is done. */
	EXPECT_EQ(run_loop(), true);
}