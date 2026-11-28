# Local development with the MCP server

`tools/mcp/grind_mcp.py` lets Claude (or any MCP client) drive the firmware without a device and read logs. It
needs Python 3 and a C compiler (`CC`, default `cc`); nothing to install. The first simulator call builds
`build/sim/host/sim_cli` from `tests/sim_cli.c`.

```sh
claude mcp add grind -- python tools/mcp/grind_mcp.py
python tests/mcp_test.py        # protocol, simulator and log tools; on Windows from a shell with cc on PATH
```

## Simulator tools

The firmware's own C (the core of the web simulator and `tests/sim_test.c`) runs as a native process. It has no
hardware access: it cannot reach an FM-1.

| Tool | Does |
|---|---|
| `sim_reset` | power on a fresh instance (`grind_native=false` for the stock path, `rebuild=true` after a source change) |
| `sim_press` | press and release a panel button (`hold` frames for a long press) |
| `sim_turn` | turn SELECT, ALGO, PRESET or K1..K4 |
| `sim_key` | play note key 0..26 |
| `sim_run` | advance the audio clock (about 48000 frames a second) so a song plays or a queued change lands |
| `sim_state` | page, mode, track, transport, on-screen message, LED masks, and under GRIND the commit count and status |
| `sim_audio` | peak and RMS of what the firmware played |
| `sim_screenshot` | the 240x240 screen as a PNG |

As on the device, a button acts in the frame after the press; `sim_press` runs the frames for you.

## Emulator tools

The simulator compiles the firmware's *source* for the PC. The emulator runs the finished *package* (`.fwsc`), the
binary that goes on the device, on an emulated FM-1 CPU: [fm1-emu](https://github.com/joakimeriksson/fm1-emu) (MIT,
not part of this repository). Download its release for your system and point `FM1_EMU` at the `fm1-emu` binary.

| Tool | Does |
|---|---|
| `emu_fetch_package` | download a release's `.fwsc` into `build/emu` with `gh` and check it against that release's `SHA256SUMS.txt` |
| `emu_run` | boot a package and play a script: `keys` (`'1000-1100:EDIT,1500:enc3:+2'`, times in ms), `snapshots` (screen times). Returns exit code, instructions, interrupts, LCD frames, audio level and the screens |

A run interprets the CPU: about 17 s of real time per emulated second, so keep `ms` short. It shows that the device
build boots, draws and plays, and it shows what a script does to it. It does not measure the RAM budget (that is the
build's map: CI's `RAM .data+.bss` line), and it is another project's model of the chip, so the last check stays a
real FM-1. `tests/mcp_test.py` runs the emulator checks when `FM1_EMU` and `FM1_EMU_PACKAGE` (a `.fwsc`) are set.

## What each backend does not check

The simulator has no CPU, so no alignment faults, stack depth, instruction timing, interrupt interplay, flash driver,
USB or Bluetooth. The emulator models those, but only as far as fm1-emu does; its batch runner has no realistic
timing mode (`FM1_STRICT` is fm1-live's).

## Log tools (read-only)

| Tool | Does |
|---|---|
| `log_summary` | time span, top events, commands, error-like events and the ends of a log (M-UPGRADE `.jsonl`, `tools/fm1_serial_log.py` captures, any text) |
| `log_read` | lines by number, or the tail |
| `log_search` | substring or regex with context; `event` filters JSON logs by event name |
| `ci_log` | a GitHub Actions run's failed log through `gh`, filtered by a regex |

None of them opens a serial port or touches a device. To capture the console, run `tools/fm1_serial_log.py` yourself
and point `log_read` at its file.
