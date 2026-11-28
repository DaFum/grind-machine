# Recovering an FM-1 on Windows (boot mode, no extra hardware)

This is the Windows version of [RECOVERY.md](RECOVERY.md) (which has the Mac version and the explanation of how it
works). Use it when the web installer **and** M-UPGRADE can no longer finish writing the FM-1, for example when
M-UPGRADE stops at "Resuming OTA recovery" at the same spot every time with `KS MIDI receive timeout error=258`. It
puts M-VAVE's stock firmware (V15) back through the chip's boot mode. The boot mode lives in the chip itself, so no
firmware can erase it.

**At your own risk.** Read each step before you run it, and stop at the first step that does not turn out as
described. The tool only reads and writes the FM-1. The boot area, the chip key and the data area (settings,
samples) are never written.

**State of verification:** The tool is tested against a simulated FM-1 (`tests/fm1_uboot_test.py`) and ran once on a
real FM-1 from a Mac. The Windows path (WinUSB driver, libusb DLL) is **not** verified on real hardware. If a step
turns out differently from what is written here, stop and report it.

## Easy way: FM1 Recovery by SikkenWorks (ready-made Windows app)

There is a ready-made Windows application with a GUI that does the same thing (restore V15 through boot mode, a
mandatory full-flash backup first, verification afterwards):
<https://github.com/SikkenWorks/FM1recovery>. It builds on the same tools as the one in this repository
(MvaveFM1Unbricker by Quixotic7 and jl-uboot-tool). It needs no Python, no Zadig and no libusb DLL: Windows holds
the boot-mode device as a drive called `WL82 UBOOT1.00 USB Device`, and the app talks to it as a disk (start it as
Administrator). **This is probably the simpler way on Windows**, and the Zadig steps below are then not needed and
probably harmful (WinUSB replaces the drive driver this app uses).

Its status: according to its own README the GUI release is not confirmed on real Windows hardware, only the earlier
command-line route ran on the developer's FM-1. I have not run the app or inspected its payload. Check the source of
the EXE yourself before you start it.

Its README says the way into boot mode works **without running firmware**: switch the FM-1 off, hold **OCT−**, switch
it on while holding, connect USB, release (the screen stays black). **This is disputed**: M-Flasher's README (below)
says a unit with the stock bootloader and stock firmware cannot enter download mode from its keys. I have verified
neither; treat the OCT− power-on route as unconfirmed on a unit running M-VAVE's own firmware. A device in boot mode
shows up in Device Manager under "Disk drives". If Windows asks whether to format a disk, always click **Cancel**.

## Another Windows way: M-Flasher

[M-Flasher](https://github.com/jvitkauskas/m-flasher) (Windows, macOS, Linux; window and command line) backs up,
restores and installs firmware on the FM-1 through the chip's USB download mode. That mode is built into the chip's ROM
and works even when the firmware does not, **but the unit has to be in it first**: M-Flasher cannot start a unit whose
firmware does not run. Per its README it saves a backup before it writes, checks that the package is for this model,
writes only what changes and verifies every write. How the unit gets into download mode:

- with the unit **running** (M-VAVE's firmware or Felucca), just connect it: M-Flasher finds it on USB MIDI and its
  **DOWNLOAD MODE** button switches it over (this also works on stock firmware);
- with Felucca, SLOOP or Snoop37 running: hold **OCT−** and **OCT+** for 5 s;
- with its own **m-bootloader already installed**: hold **REC** while switching on. This is the key route that still
  works when the firmware does not start;
- a unit whose firmware is damaged often ends up in download mode by itself (per M-Flasher's usage doc), so connect
  it and see whether M-Flasher finds it.

**If the firmware does not start and the unit is not in download mode**, and it still has M-VAVE's stock bootloader
(no m-bootloader), there is no key route: M-Flasher's usage doc says its keys cannot put such a unit into download
mode, and that the USB_KEY dongle (a Raspberry Pi Pico) still reaches it. The FM-1 Transporter mentioned below is
the same kind of hardware route. M-Flasher offers to install m-bootloader once per unit, which gives the unit a key
route for next time.

The apps are not signed, so Windows and Defender warn on the first start. Its README gives the licence as MIT, with
some bundled files under their own licences (listed in its `data/NOTICE.md` and `assets/README.md`). I have not run it.
The same goes for
[Slipway](https://github.com/Discodave-Glitch/slipway), a Windows app with a rescue flow for a black-screen FM-1.

The rest of this guide describes the route with the tool from this repository (libusb instead of a disk).

## What you need

- Windows 10 or 11 (64-bit), a USB data cable that has worked with the FM-1 before, plugged straight into the PC
  (no hub).
- [Python 3](https://www.python.org/downloads/) (64-bit) with the `py` launcher.
- [Git](https://git-scm.com/download/win) (or the ZIP file of this repository).
- [Zadig](https://zadig.akeo.ie/) (installs the WinUSB driver for the device).
- The 64-bit `libusb-1.0.dll` from the releases at <https://github.com/libusb/libusb/releases> (file
  `libusb-*.7z`, inside it `VS2022\MS64\dll\libusb-1.0.dll`; unpack with 7-Zip).
- M-VAVE's stock firmware `FM-1.fwsc` (V15) from <https://www.m-vave.com/download> (Downloads, PC Firmware,
  FM-1 V15).
- A charged FM-1.

All commands are for **PowerShell** and are run in the folder of this repository (`cd` there).

## 0. One-time preparation

```powershell
py -3 -m venv $HOME\fm1-rescue
& $HOME\fm1-rescue\Scripts\pip install pyusb
git clone https://github.com/kagaimiq/jl-uboot-tool $HOME\jl-uboot-tool
Get-Item $HOME\jl-uboot-tool\data\loaderblobs\usb\wl82loader.bin      # length 24064
Get-FileHash -Algorithm SHA256 $HOME\Downloads\FM-1.fwsc            # must start with db1642b2
```

The tool refuses every other firmware file. Put the unpacked `libusb-1.0.dll` somewhere such as
`C:\fm1-rescue\libusb-1.0.dll` and shorten the commands:

```powershell
$env:FM1_LIBUSB = "C:\fm1-rescue\libusb-1.0.dll"
$RESCUE = "$HOME\fm1-rescue\Scripts\python.exe tools\rescue\fm1_uboot.py"
$LOADER = "$HOME\jl-uboot-tool\data\loaderblobs\usb\wl82loader.bin"
```

(`$env:FM1_LIBUSB` and the two variables only last for this PowerShell window. If you open a new one, set them
again.)

## 1. Put the FM-1 into boot mode

Use one of these:

- **The firmware is running** (Hortator, Felucca or GRIND): hold **OCT−** and **OCT+** together. After 2 s a
  countdown appears; letting go cancels it. At 5 s the screen shows `UBOOT`.
- **From the computer:** send the SysEx message `F0 22 24 35 7D F7` to the FM-1's MIDI port (works with M-VAVE's
  V15 too). A Windows program for that is MIDI-OX, for example; I have not tried it this way.
- **Without running firmware (unconfirmed):** switch the FM-1 off, hold **OCT−**, switch it on while holding,
  connect USB, then release. The SikkenWorks project describes it; M-Flasher's README says a unit with the stock
  bootloader and stock firmware cannot do this from its keys. I have verified neither.
- **The firmware crashes right after start:** two crashes within 30 s of starting lead into boot mode by
  themselves (Felucca-based firmware).

The screen goes black or keeps the last picture. The FM-1 stays in boot mode until you switch it off or the tool is
finished. Check **Device Manager**: under "Disk drives", "USB mass storage devices" or "Other devices" a device
**`WL80UBOOT1.00`** (USB ID `4C4A:8057`) should appear. Close M-UPGRADE, the web installer and every program that
uses the FM-1 over MIDI first.

If the FM-1 reports itself neither normally nor as `WL80UBOOT`, this guide does not help. What remains is the
[FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) (a Seeed XIAO RP2040 on the FM-1's USB lines),
which forces boot mode even without running firmware.

## 2. Driver for the boot-mode device (one time)

Windows loads its mass-storage driver for the device. The tool needs **WinUSB** instead:

1. Leave the FM-1 in boot mode on the PC (step 1) and start Zadig.
2. Switch on the menu **Options → List All Devices**.
3. In the list choose the device with USB ID **`4C4A 8057`** (name `WL80UBOOT1.00` or similar). **Do not choose any
   other device**: Zadig replaces the driver of the device you choose.
4. On the right set the target driver to **WinUSB** and click **Replace Driver**.

After that the device can be reached by the tool. The driver stays in place for this USB ID. If you later want a
drive letter for the boot-mode device again, undo it in Device Manager with "Driver → Roll Back Driver". Normal
FM-1 operation (MIDI, installer) is not affected.

## 3. Back up the whole flash (read only)

```powershell
Invoke-Expression "$RESCUE backup --loader $LOADER $HOME\fm1-backup.bin"
```

Expected output:

```
chip key 0x980F, flash type 3 id 0x856014
read 1: 2.3 s, sha256 …
read 2: 2.3 s, sha256 …   (same sum as read 1)
saved C:\Users\you\fm1-backup.bin; nothing was written to the FM-1
```

- `fm1_uboot: libusb not found`: `$env:FM1_LIBUSB` does not point at the DLL (or it is 32-bit while Python is
  64-bit).
- `no WL80UBOOT (4C4A:8057) device`: the FM-1 is not in boot mode, or Zadig did not get the right device (step 2).
- `cannot take the device …`: the WinUSB driver is missing (step 2).
- `not an FM-1 as expected`: stop here. Nothing was read or written.
- `the two reads differ`: try once more, and if it repeats, use another cable.

After the run the FM-1 restarts and boots its firmware as before. Keep `fm1-backup.bin` somewhere safe: it is the
flash of this FM-1 including its data area.

## 4. Check the restore (writes nothing)

Put the FM-1 into boot mode again (step 1), then:

```powershell
Invoke-Expression "$RESCUE restore --loader $LOADER $HOME\Downloads\FM-1.fwsc $HOME\fm1-backup.bin"
```

Expected: the chip line again, `143 of 143 app sectors differ from stock V15` (fewer if part of it is already
stock) and `DRY RUN: nothing written (add --write)`.

The check refuses when:

- **the package is not V15** (`is not M-VAVE's stock V15 FM-1.fwsc`): download the file from M-VAVE again.
- **the backup does not match this FM-1** (`head or data differ from the backup`): make a new backup (step 3).

## 5. Write the stock firmware

Put the FM-1 into boot mode again (step 1), then:

```powershell
Invoke-Expression "$RESCUE restore --loader $LOADER $HOME\Downloads\FM-1.fwsc $HOME\fm1-backup.bin --write"
```

Do not pull the cable and do not switch the FM-1 off until `done` appears (about 15 s). A progress line comes every
16 sectors, and at the end:

```
done: stock V15 app written and verified. Unplug USB or power-cycle the FM-1.
```

- `verify failed, again`: one retry, you do not have to do anything.
- `failed 3 times: STOP` or `FINAL CHECK FAILED`: leave the FM-1 switched on and in boot mode and run the same
  command again (if it has stopped, put it into boot mode again first). Only the sectors that are not yet stock
  are written, so running it again is safe. The same goes for an interrupted run, for example with a pulled cable.

## 6. Start the FM-1 and reinstall your firmware

1. Unplug USB or switch the FM-1 off and on. It starts with M-VAVE's stock firmware.
2. Then install the firmware you want again through the web installer:
   <https://dafum.github.io/grind-machine/webapp/installer/> (Chrome or Edge, over USB).

The data area above `0x93000` (settings, presets, your own samples) was not touched: it is as it is in your backup.
