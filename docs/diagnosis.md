# Technical diagnosis

## Summary

On the tested Lenovo Yoga S740-14IIL (`81RS`), bare F11/F12 brightness functions produced no Linux input event even though the backlight device and desktop brightness control worked. The firmware exposes the expected ACPI query methods, but physical presses do not cause Linux to execute them through the normal EC notification path.

Evaluating the firmware's `_Q44` method periodically makes the physical keys produce native ACPI Video Bus brightness events. This repository packages that behaviour as a DMI-restricted DKMS module.

## Baseline system

```text
DMI product: 81RS
Product version: Yoga S740-14IIL
Board: LNVNB161216
BIOS: BYCN39WW, 2021-05-28
Ubuntu: 26.04
Kernel: 7.0.0-28-generic
Session: Wayland
Backlight: /sys/class/backlight/intel_backlight
```

The backlight device reported type `raw` and maximum brightness `21333`. GNOME Mutter's native backlight D-Bus operation changed and restored the physical panel brightness successfully.

## Input testing

Physical bare brightness presses were tested on:

- AT Translated Set 2 keyboard
- Intel HID events
- Ideapad extra buttons
- both ACPI Video Bus devices

No device emitted an event. Fn+F11/F12 did emit ordinary function-key behaviour, confirming that the terminal escape observed from Fn+F12 was an F12 event, not a brightness event.

The embedded-controller GPE counter did not increment for bare brightness presses. Watchers of the DSDT's shared EC memory region at `0xFE0B0400` and its private indexed interface at I/O ports `0x72/0x73` also showed no press-related change in the exposed bytes.

## ACPI state

Live evaluation through the kernel ACPI debugger returned:

```text
\OSYS                                 = 0x07DF
\_SB.PCI0.LPCB.EC0.OSTY              = 6
\_SB.PCI0.LPCB.EC0.ECAV              = 1
\_SB.PCI0.LPCB.EC0.VPC0.HALS         = 0xCAF8
```

Thus:

- the EC operation region was active;
- the firmware saw the Windows-2015 compatibility level it expects;
- hotkey mode was already correct;
- reasserting the existing Fn-lock state did not help.

This ruled out a practical `_OSI`/OSYS override or Fn-lock initialization fix.

## Relevant DSDT methods

The firmware defines:

```asl
Method (_Q11, 0, NotSerialized)
{
    P80B = 0x11
    Notify (^^^GFX0.DD1F, 0x87)
    Notify (VPC0, 0x80)
}

Method (_Q12, 0, NotSerialized)
{
    P80B = 0x12
    Notify (^^^GFX0.DD1F, 0x86)
    Notify (VPC0, 0x80)
}

Method (_Q44, 0, NotSerialized)
{
    P80B = 0x44
    Notify (VPC0, 0x80)
}
```

ACPI notification `0x87` means brightness down, while `0x86` means brightness up.

Manually evaluating `_Q11` and `_Q12` produced native `KEY_BRIGHTNESSDOWN` and `KEY_BRIGHTNESSUP` events on the integrated Video Bus. GNOME acted on those events correctly. Therefore the full downstream path was healthy.

## Observed repair behaviour

Evaluating `_Q44` prompts the existing `ideapad_laptop` notification handler to read the Lenovo VPC registers through `VPCR`. Repeating `_Q44` at a short interval makes physical bare brightness presses produce the expected native ACPI video events.

The repair was confirmed at these intervals:

| Interval | Polls/second | Result |
|---:|---:|---|
| 50 ms | 20 | Working |
| 100 ms | 10 | All requested native events captured |
| 250 ms | 4 | All requested native events captured |

The public default is 100 ms as a balance between responsiveness and polling frequency.

## Reboot and login-screen validation

The DKMS installation was reboot-tested on Ubuntu kernel `7.0.0-29-generic`. Module version `1.0.0` loaded automatically with `poll_interval_ms=100`, and both brightness keys worked on the graphical login screen before a user session started. They continued to work after signing in to GNOME. This confirms that the repair does not depend on a per-user shortcut, login script, or desktop-session helper.

## Observation versus inference

Observed:

- Physical presses are silent without periodic VPC servicing.
- `_Q11`/`_Q12` produce correct native events when invoked.
- Periodic `_Q44` evaluation makes physical presses work.
- `_Q44` contains only a POST-byte write and `Notify(VPC0, 0x80)` in this firmware.

Inferred:

- The EC retains a brightness query or related state but fails to raise the SCI/GPE that normally causes Linux to service it.
- The VPC reads triggered after `_Q44` give the EC driver an opportunity to notice and drain that pending state.

The module deliberately describes the internal mechanism conservatively because no EC firmware source or hardware trace was available.

## Rejected approaches

- `brightnessctl` plus desktop custom shortcuts
- F11/F12 remapping
- `acpi_backlight=vendor`
- Fn-lock toggling or reinitialization
- `_OSI` overrides
- changing the already-functional `intel_backlight` path

These either did not receive a physical event or addressed a layer already proven to work.

## Upstream considerations

This out-of-tree module is useful for affected users but is not necessarily the form an upstream maintainer will accept. Questions for `platform-driver-x86` include:

- whether polling a firmware query method belongs in `ideapad_laptop`;
- whether a lower-level ACPI EC mechanism can detect the missing notification more efficiently;
- whether other `81RS` BIOS versions behave identically;
- the measurable idle-power impact at 100 ms and 250 ms;
- whether the quirk should require both DMI product and BIOS constraints.

An upstream RFC should ask for design guidance instead of presenting the polling module as a finished mainline patch.
