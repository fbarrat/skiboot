/* Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <skiboot.h>
#include <device.h>
#include <console.h>
#include <chip.h>
#include <ipmi.h>
#include <psi.h>
#include <npu-regs.h>
#include <xscom.h>
#include <xscom-p9-regs.h>
#include <timebase.h>
#include <pci.h>
#include <pci-slot.h>
#include <phb4.h>

#include "astbmc.h"


const struct platform_ocapi witherspoon_ocapi = {
	.i2c_engine	= 1,
	.i2c_port	= 4,
	.i2c_reset_addr = 0x20,
	/*
	 * Opencapi reset on witherspoon is triggered through i2c,by
	 * using the GPU_INT pins. There are 3 pins per CPU, one for
	 * each GPU (GPU0, GPU1, GPU2). On redbud, we only have GPU0
	 * and GPU1.
	 *
	 * Only the "middle" lane group is compatible with the Acorn
	 * adapter. On obus0, it maps to ODL0 and GPU0, and on obus3,
	 * it maps to ODL1 and GPU1. So we get lucky, an odl number
	 * always identify a unique GPU/Acorn slot.
	 */
	.i2c_reset_odl0 = (1 << 0), /* GPU0 */
	.i2c_reset_odl1 = (1 << 1), /* GPU1 */
	.force_presence = true,
	.odl_phy_swap	= false,
};


/*
 * HACK: Hostboot doesn't export the correct data for the system VPD EEPROM
 *       for this system. So we need to work around it here.
 */
static void vpd_dt_fixup(void)
{
	struct dt_node *n = dt_find_by_path(dt_root,
		"/xscom@603fc00000000/i2cm@a2000/i2c-bus@0/eeprom@50");

	if (n) {
		dt_check_del_prop(n, "compatible");
		dt_add_property_string(n, "compatible", "atmel,24c512");

		dt_check_del_prop(n, "label");
		dt_add_property_string(n, "label", "system-vpd");
	}
}

static void create_ocapi_i2c_bus(void)
{
	struct dt_node *xscom, *i2cm, *i2c_bus;

	prlog(PR_DEBUG, "OCAPI: Adding i2c bus to device tree\n");

	dt_for_each_compatible(dt_root, xscom, "ibm,xscom") {
		i2cm = dt_find_by_name(xscom, "i2cm@a1000");
		if (!i2cm) {
			prlog(PR_ERR, "OCAPI: i2c master for adapter reset not found\n");
			continue;
		}

		if (dt_find_by_name(i2cm, "i2c-bus@4"))
			continue;

		i2c_bus = dt_new_addr(i2cm, "i2c-bus", 4);
		dt_add_property_cells(i2c_bus, "reg", 4);
		dt_add_property_cells(i2c_bus, "bus-frequency", 0x61a80);
		dt_add_property_strings(i2c_bus, "compatible",
					"ibm,opal-i2c", "ibm,power8-i2c-port",
					"ibm,power9-i2c-port");
	}
}

static void fix_ocapi_link(void)
{
	struct dt_node *link;
	struct dt_property *prop;

	prlog(PR_INFO, "OCAPI: making GPU1 slot opencapi-compatible\n");

	link = dt_find_by_name(dt_root, "link@4");
	if (!link) {
		prlog(PR_ERR, "OCAPI: cannot find link4 to modify\n");
		return;
	}

	/*
	 * On redbud, GPU1 is the middle-group lanes on obus3. The
	 * lane mask we get from hostboot for link 4 is correct.
	 */

	/* fix link type */
	prop = __dt_find_property(link, "compatible");
	if (!prop) {
		prlog(PR_ERR, "OCAPI: cannot find compatible prop in link\n");
		return;
	}
	dt_del_property(link, prop);
	dt_add_property_string(link, "compatible", "ibm,npu-link-opencapi");
}

static bool witherspoon_probe(void)
{
	if (!dt_node_is_compatible(dt_root, "ibm,witherspoon"))
		return false;

	/* Lot of common early inits here */
	astbmc_early_init();

	/* Setup UART for use by OPAL (Linux hvc) */
	uart_set_console_policy(UART_CONSOLE_OPAL);

	vpd_dt_fixup();

	/* opencapi fixup */
	create_ocapi_i2c_bus();
	fix_ocapi_link();
	return true;
}

static void phb4_activate_shared_slot_witherspoon(struct proc_chip *chip)
{
	uint64_t val;

	/*
	 * Shared slot activation is done by raising a GPIO line on the
	 * chip with the secondary slot. It will somehow activate the
	 * sideband signals between the slots.
	 * Need to wait 100us for stability.
	 */
	xscom_read(chip->id, P9_GPIO_DATA_OUT_ENABLE, &val);
	val |= PPC_BIT(2);
	xscom_write(chip->id, P9_GPIO_DATA_OUT_ENABLE, val);

	xscom_read(chip->id, P9_GPIO_DATA_OUT, &val);
	val |= PPC_BIT(2);
	xscom_write(chip->id, P9_GPIO_DATA_OUT, val);
	time_wait_us(100);
	prlog(PR_INFO, "Shared PCI slot activated\n");
}

static void phb4_pre_pci_fixup_witherspoon(void)
{
	struct pci_slot *slot0, *slot1;
	struct proc_chip *chip0, *chip1;
	uint8_t p0 = 0, p1 = 0;

	/*
	 * Detect if a x16 card is present on the shared slot and
	 * do some extra configuration if it is.
	 *
	 * The shared slot, a.k.a "Slot 2" in the documentation, is
	 * connected to PEC2 phb index 3 on both chips. From skiboot,
	 * it looks like two x8 slots, each with its own presence bit.
	 *
	 * Here is the matrix of possibilities for the presence bits:
	 *
	 * slot0 presence     slot1 presence
	 *    0                  0               => no card
	 *    1                  0               => x8 or less card detected
	 *    1                  1               => x16 card detected
	 *    0                  1               => invalid combination
	 *
	 * We only act if a x16 card is detected ('1 1' combination above).
	 *
	 * One issue is that we don't really know if it is a
	 * shared-slot-compatible card (such as Mellanox CX5) or
	 * a 'normal' x16 PCI card. We activate the shared slot in both cases,
	 * as it doesn't seem to hurt.
	 *
	 * If the card is a normal x16 PCI card, the link won't train on the
	 * second slot (nothing to do with the shared slot activation), the
	 * procedure will timeout, thus adding some delay to the boot time.
	 * Therefore the recommendation is that we shouldn't use a normal
	 * x16 card on the shared slot of a witherspoon.
	 *
	 * Plugging a x8 or less adapter on the shared slot should work
	 * like any other physical slot.
	 */
	chip0 = next_chip(NULL);
	chip1 = next_chip(chip0);
	if (!chip1 || next_chip(chip1)) {
		prlog(PR_WARNING,
			"Unexpected number of chips, skipping shared slot detection\n");
		return;
	}

	/* the shared slot is connected to PHB3 on both chips */
	slot0 = pci_slot_find(phb4_get_opal_id(chip0->id, 3));
	slot1 = pci_slot_find(phb4_get_opal_id(chip1->id, 3));
	if (slot0 && slot1) {
		if (slot0->ops.get_presence_state)
			slot0->ops.get_presence_state(slot0, &p0);
		if (slot1->ops.get_presence_state)
			slot1->ops.get_presence_state(slot1, &p1);
		if (p0 == 1 && p1 == 1) {
			phb4_activate_shared_slot_witherspoon(chip1);
			slot0->peer_slot = slot1;
			slot1->peer_slot = slot0;
		}
	}
}

static void witherspoon_pre_pci_fixup(void)
{
	phb4_pre_pci_fixup_witherspoon();
}

/* The only difference between these is the PCI slot handling */

DECLARE_PLATFORM(witherspoon) = {
	.name			= "Witherspoon",
	.probe			= witherspoon_probe,
	.init			= astbmc_init,
	.pre_pci_fixup		= witherspoon_pre_pci_fixup,
	.start_preload_resource	= flash_start_preload_resource,
	.resource_loaded	= flash_resource_loaded,
	.bmc			= &astbmc_openbmc,
	.cec_power_down         = astbmc_ipmi_power_down,
	.cec_reboot             = astbmc_ipmi_reboot,
	.elog_commit		= ipmi_elog_commit,
	.exit			= ipmi_wdt_final_reset,
	.terminate		= ipmi_terminate,

	.pci_get_slot_info	= dt_slot_get_slot_info,
	.ocapi			= &witherspoon_ocapi,
};
