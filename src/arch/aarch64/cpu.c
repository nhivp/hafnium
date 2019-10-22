/*
 * Copyright 2018 The Hafnium Authors.
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

#include "hf/arch/cpu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hf/addr.h"
#include "hf/spci.h"
#include "hf/std.h"

#include "hypervisor/perfmon.h"
#include "hypervisor/sysregs.h"

void arch_irq_disable(void)
{
	__asm__ volatile("msr DAIFSet, #0xf");
}

void arch_irq_enable(void)
{
	__asm__ volatile("msr DAIFClr, #0xf");
}

static void gic_regs_reset(struct arch_regs *r, bool is_primary)
{
#if GIC_VERSION == 3 || GIC_VERSION == 4
	uint32_t ich_hcr = 0;
	uint32_t icc_sre_el2 =
		(1U << 0) | /* SRE, enable ICH_* and ICC_* at EL2. */
		(0x3 << 1); /* DIB and DFB, disable IRQ/FIQ bypass. */

	if (is_primary) {
		icc_sre_el2 |= 1U << 3; /* Enable EL1 access to ICC_SRE_EL1. */
	} else {
		/* Trap EL1 access to GICv3 system registers. */
		ich_hcr =
			(0x1fU << 10); /* TDIR, TSEI, TALL1, TALL0, TC bits. */
	}
	r->gic.ich_hcr_el2 = ich_hcr;
	r->gic.icc_sre_el2 = icc_sre_el2;
#endif
}

void arch_regs_reset(struct arch_regs *r, bool is_primary, spci_vm_id_t vm_id,
		     cpu_id_t vcpu_id, paddr_t table)
{
	uintreg_t pc = r->pc;
	uintreg_t arg = r->r[0];
	uintreg_t cptr;
	uintreg_t cnthctl;

	memset_s(r, sizeof(*r), 0, sizeof(*r));

	r->pc = pc;
	r->r[0] = arg;

	cptr = 0;
	cnthctl = 0;

	if (is_primary) {
		cnthctl |=
			(1U << 0) | /* EL1PCTEN, don't trap phys cnt access. */
			(1U << 1);  /* EL1PCEN, don't trap phys timer access. */
	}

	r->lazy.hcr_el2 = get_hcr_el2_value(vm_id);
	r->lazy.cptr_el2 = cptr;
	r->lazy.cnthctl_el2 = cnthctl;
	r->lazy.vttbr_el2 = pa_addr(table) | ((uint64_t)vm_id << 48);
	r->lazy.vmpidr_el2 = vcpu_id;
	/* TODO: Use constant here. */
	r->spsr = 5 |	 /* M bits, set to EL1h. */
		  (0xf << 6); /* DAIF bits set; disable interrupts. */

	r->lazy.mdcr_el2 = get_mdcr_el2_value(vm_id);

	/*
	 * NOTE: It is important that MDSCR_EL1.MDE (bit 15) is set to 0 for
	 * secondary VMs as long as Hafnium does not support debug register
	 * access for secondary VMs. If adding Hafnium support for secondary VM
	 * debug register accesses, then on context switches Hafnium needs to
	 * save/restore EL1 debug register state that either might change, or
	 * that needs to be protected.
	 */
	r->lazy.mdscr_el1 = 0x0U & ~(0x1U << 15);

	/* Disable cycle counting on initialization. */
	r->lazy.pmccfiltr_el0 = perfmon_get_pmccfiltr_el0_init_value(vm_id);

	gic_regs_reset(r, is_primary);
}

void arch_regs_set_pc_arg(struct arch_regs *r, ipaddr_t pc, uintreg_t arg)
{
	r->pc = ipa_addr(pc);
	r->r[0] = arg;
}

void arch_regs_set_retval(struct arch_regs *r, struct spci_value v)
{
	r->r[0] = v.func;
	r->r[1] = v.arg1;
	r->r[2] = v.arg2;
	r->r[3] = v.arg3;
	r->r[4] = v.arg4;
	r->r[5] = v.arg5;
	r->r[6] = v.arg6;
	r->r[7] = v.arg7;
}
