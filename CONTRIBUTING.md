# Contributing

Reports from other Lenovo Yoga S740-14IIL owners are especially useful.

Please include:

```bash
cat /sys/class/dmi/id/sys_vendor
cat /sys/class/dmi/id/product_name
cat /sys/class/dmi/id/product_version
cat /sys/class/dmi/id/bios_version
uname -r
```

Also report:

- whether the keys work at the default 100 ms interval;
- whether 250 ms works without missed presses;
- desktop environment and session type;
- suspend/resume behaviour;
- any relevant `dmesg` lines containing `yoga_s740_vpc_poll`, `ideapad`, or `ACPI`;
- whether Secure Boot is enabled.

Do not include a device serial number, UUID, email address, or other private identifier.

Kernel changes should follow Linux kernel coding style. Run a clean build against the target kernel headers before submitting a pull request.
