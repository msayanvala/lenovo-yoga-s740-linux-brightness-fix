# Lenovo Yoga S740-14IIL Linux brightness-key fix

This repository contains a model-specific Linux kernel workaround for display-brightness keys that produce no input event on the Lenovo Yoga S740-14IIL, DMI product `81RS`.

The tested laptop uses Lenovo BIOS `BYCN39WW`. Its backlight slider and Intel backlight interface work normally, but bare brightness-down and brightness-up presses are silent on the keyboard, Intel HID, Ideapad, and ACPI Video Bus input devices.

The module periodically evaluates the firmware's notification-only `_Q44` method. This prompts the existing `ideapad_laptop` and ACPI-video paths to service the Lenovo VPC/embedded-controller state, after which Linux emits native `KEY_BRIGHTNESSDOWN` and `KEY_BRIGHTNESSUP` events. GNOME then changes `intel_backlight` normally.

This is a community workaround, not an upstream kernel fix.

## Safety and scope

- The module refuses to load unless DMI reports manufacturer `LENOVO` and exact product name `81RS`.
- It does not write brightness values directly.
- It does not remap F11/F12 or install userspace shortcuts.
- Its workqueue freezes during suspend.
- The default interval is 100 ms. Tests on the original machine captured all requested events at 50, 100, and 250 ms.
- Periodic AML/EC evaluation may have a power cost. It has not yet been quantified.
- Do not install this on another Lenovo model by removing the DMI guard. Other firmware can assign a different meaning to `_Q44`.

## Tested configuration

| Item | Tested value |
|---|---|
| Laptop | Lenovo Yoga S740-14IIL |
| DMI product | `81RS` |
| BIOS | `BYCN39WW` |
| Distribution | Ubuntu 26.04 |
| Kernel | `7.0.0-28-generic` |
| Desktop | GNOME 50, Wayland |
| Graphics | Intel `i915` with `intel_backlight` |

DKMS builds were also completed successfully for Ubuntu kernel `7.0.0-29-generic`; a reboot test on that kernel is still pending.

## Before installing

Confirm the exact DMI product:

```bash
cat /sys/class/dmi/id/sys_vendor
cat /sys/class/dmi/id/product_name
```

The output must be:

```text
LENOVO
81RS
```

Install the build dependencies on Ubuntu or Debian:

```bash
sudo apt install dkms build-essential "linux-headers-$(uname -r)"
```

Secure Boot must either be disabled or configured to trust the key used by DKMS to sign locally built modules.

## Install

```bash
git clone https://github.com/msayanvala/lenovo-yoga-s740-linux-brightness-fix.git
cd lenovo-yoga-s740-linux-brightness-fix
sudo ./install.sh
```

The installer validates DMI and the running kernel's headers before writing anything. It then:

1. places the source in `/usr/src/yoga-s740-vpc-poll-1.0.0`;
2. registers and builds it with DKMS for the running kernel;
3. installs `/etc/modules-load.d/yoga-s740-vpc-poll.conf`;
4. loads the module immediately.

The keys should work without rebooting.

## Verify

```bash
lsmod | grep '^yoga_s740_vpc_poll'
dkms status -m yoga-s740-vpc-poll
modinfo -F filename yoga_s740_vpc_poll
sudo dmesg | grep yoga_s740_vpc_poll
```

The load message should resemble:

```text
yoga_s740_vpc_poll: enabled for 81RS, interval=100 ms
```

To verify native input events, run `sudo evtest`, select the integrated ACPI `Video Bus`, and press the bare brightness keys. Event numbers can change between boots. Expected event names are:

```text
KEY_BRIGHTNESSDOWN
KEY_BRIGHTNESSUP
```

## Change the polling interval

The supported range is 20–250 ms. The default and recommended starting value is 100 ms.

For a one-time test:

```bash
sudo modprobe -r yoga_s740_vpc_poll
sudo modprobe yoga_s740_vpc_poll poll_interval_ms=250
```

To make another value persistent, create `/etc/modprobe.d/yoga-s740-vpc-poll.conf` containing, for example:

```text
options yoga_s740_vpc_poll poll_interval_ms=250
```

Then reload the module or reboot. A larger interval reduces polling frequency but can add latency.

## Kernel updates

`AUTOINSTALL="yes"` in `dkms.conf` asks DKMS to build the module for future kernels when matching headers are installed.

After an update:

```bash
dkms status -m yoga-s740-vpc-poll
```

If the running kernel is absent, install its headers and run:

```bash
sudo dkms autoinstall
```

## Remove

Unload the module first:

```bash
sudo modprobe -r yoga_s740_vpc_poll
```

Then remove the exact installation:

```bash
sudo rm /etc/modules-load.d/yoga-s740-vpc-poll.conf
sudo dkms remove yoga-s740-vpc-poll/1.0.0 --all
sudo rm -r /usr/src/yoga-s740-vpc-poll-1.0.0
```

If you created a persistent parameter file, also remove:

```bash
sudo rm /etc/modprobe.d/yoga-s740-vpc-poll.conf
```

## What was diagnosed

The important findings were:

- `intel_backlight` and GNOME's native brightness control worked.
- Bare brightness presses generated no event on every relevant input device.
- The firmware had the correct Fn-lock, EC-availability, OSYS, and OSTY state.
- The DSDT defines `_Q11`/`_Q12` with the standard ACPI brightness notifications.
- Manually evaluating `_Q11`/`_Q12` generated correct native Video Bus events.
- Servicing the VPC path through `_Q44` made physical brightness presses work.

The exact internal queueing mechanism remains an inference. See [docs/diagnosis.md](docs/diagnosis.md) for the evidence and the boundary between observation and inference.

## Related reports and source material

- [Ubuntu Launchpad bug #1872311](https://bugs.launchpad.net/ubuntu/+source/linux/+bug/1872311) reports the same missing brightness events on product `81RS`.
- [Upstream Linux `ideapad-laptop.c`](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/lenovo/ideapad-laptop.c)
- [Same-model OpenCore configuration](https://github.com/nan1jueze/YOGA_S740-14IIL_i5-1035G1_OpenCore)
- [Acidanthera BrightnessKeys](https://github.com/acidanthera/BrightnessKeys), which documents ACPI video notification values `0x86` and `0x87`

## License

GPL-2.0-only. See [LICENSE](LICENSE).
