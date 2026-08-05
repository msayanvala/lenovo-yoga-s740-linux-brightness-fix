// SPDX-License-Identifier: GPL-2.0-only
/*
 * Lenovo Yoga S740-14IIL (81RS) brightness-hotkey notification workaround.
 *
 * On the tested BYCN39WW firmware, bare brightness-key presses produce no
 * input event until the Lenovo VPC/EC path is serviced. Evaluating the
 * firmware's notification-only _Q44 method prompts the existing
 * ideapad_laptop and ACPI-video drivers to deliver native brightness events.
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#define DRIVER_NAME "yoga_s740_vpc_poll"
#define EC_PATH "\\_SB.PCI0.LPCB.EC0"

static unsigned int poll_interval_ms = 100;
module_param(poll_interval_ms, uint, 0444);
MODULE_PARM_DESC(poll_interval_ms, "VPC poll interval in milliseconds (20-250)");

static acpi_handle ec_handle;
static acpi_handle q44_handle;
static struct delayed_work poll_work;
static bool stopping;

static const struct dmi_system_id supported_systems[] = {
	{
		.ident = "Lenovo Yoga S740-14IIL (81RS)",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "81RS"),
		},
	},
	{ }
};
MODULE_DEVICE_TABLE(dmi, supported_systems);

static void poll_vpc(struct work_struct *work)
{
	acpi_status status;

	/* _Q44 only writes a POST byte and performs Notify(VPC0, 0x80). */
	status = acpi_evaluate_object(ec_handle, "_Q44", NULL, NULL);
	if (ACPI_FAILURE(status))
		pr_warn_ratelimited(DRIVER_NAME ": _Q44 failed: %s\n",
				    acpi_format_exception(status));

	if (!READ_ONCE(stopping))
		queue_delayed_work(system_freezable_power_efficient_wq, &poll_work,
				   msecs_to_jiffies(poll_interval_ms));
}

static int __init yoga_s740_vpc_poll_init(void)
{
	acpi_status status;

	if (!dmi_check_system(supported_systems))
		return -ENODEV;

	if (poll_interval_ms < 20 || poll_interval_ms > 250) {
		pr_err(DRIVER_NAME ": poll_interval_ms must be 20..250\n");
		return -EINVAL;
	}

	status = acpi_get_handle(NULL, EC_PATH, &ec_handle);
	if (ACPI_FAILURE(status)) {
		pr_err(DRIVER_NAME ": cannot find %s: %s\n", EC_PATH,
		       acpi_format_exception(status));
		return -ENODEV;
	}

	status = acpi_get_handle(ec_handle, "_Q44", &q44_handle);
	if (ACPI_FAILURE(status)) {
		pr_err(DRIVER_NAME ": firmware has no _Q44: %s\n",
		       acpi_format_exception(status));
		return -ENODEV;
	}

	WRITE_ONCE(stopping, false);
	INIT_DELAYED_WORK(&poll_work, poll_vpc);
	queue_delayed_work(system_freezable_power_efficient_wq, &poll_work, 0);
	pr_info(DRIVER_NAME ": enabled for 81RS, interval=%u ms\n",
		poll_interval_ms);
	return 0;
}

static void __exit yoga_s740_vpc_poll_exit(void)
{
	WRITE_ONCE(stopping, true);
	cancel_delayed_work_sync(&poll_work);
	pr_info(DRIVER_NAME ": disabled\n");
}

module_init(yoga_s740_vpc_poll_init);
module_exit(yoga_s740_vpc_poll_exit);

MODULE_AUTHOR("OpenAI Codex and contributors");
MODULE_DESCRIPTION("Lenovo Yoga S740-14IIL brightness-hotkey notification workaround");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
