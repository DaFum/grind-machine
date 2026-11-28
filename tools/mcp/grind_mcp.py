#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""MCP server for local GRIND MACHINE development (stdio, no dependencies beyond Python 3).

Two groups of tools:

  simulator   Drives tests/sim_cli.c: the firmware's own C (the same core as the web simulator and tests/sim_test.c)
              as a native process. Press buttons, turn encoders, play keys, advance time, read the state, the
              screen (PNG) and the audio level. It has no hardware access of any kind: it cannot reach an FM-1.
  emulator    Runs a finished firmware package (.fwsc) in joakimeriksson/fm1-emu (MIT), which emulates the FM-1's CPU, so
              the binary that goes on the device is what runs. Set FM1_EMU to its fm1-emu binary. A run is a script
              (keys, snapshot times) played from power-on; it never touches a device.
  logs        Read-only: summarise, page and search a log file (M-UPGRADE .jsonl, a fm1_serial_log.py capture, any
              text), and fetch a failed GitHub Actions log with `gh`. Nothing here opens a serial port or a device.

Register it (Claude Code):  claude mcp add grind -- python tools/mcp/grind_mcp.py
Or in .mcp.json:            {"mcpServers": {"grind": {"command": "python", "args": ["tools/mcp/grind_mcp.py"]}}}
Needs a C compiler (CC, default `cc`) and Python 3; the first simulator call builds build/sim/host/sim_cli.
"""
import base64
import collections
import json
import os
import re
import hashlib
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / "build" / "sim" / "host" / ("sim_cli.exe" if os.name == "nt" else "sim_cli")
GEN = ROOT / "build" / "sim" / "gen"
MAX_TEXT = 24000                      # characters a tool returns at most (the rest is said to be cut)

BUTTONS = ["FX", "SCL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC", "OCTDN", "OCTUP"]
QUIET = {"stderr", "command.start", "process.start"}   # (their fields name a timeout: not a failure)
ENCODERS = ["SELECT", "ALGO", "PRESET", "K1", "K2", "K3", "K4"]


# ---------------------------------------------------------------- simulator process

class Sim:
    proc = None
    native = True

    @classmethod
    def build(cls, native=True):
        GEN.mkdir(parents=True, exist_ok=True)
        EXE.parent.mkdir(parents=True, exist_ok=True)
        code = ("import sys; sys.path.insert(0, 'tools'); import build; from pathlib import Path; "
                "build.GEN = Path(sys.argv[1]).resolve(); build.generate()")
        subprocess.run([sys.executable, "-c", code, str(GEN)], cwd=ROOT, check=True, capture_output=True)
        text = (ROOT / "firmware" / "hal" / "fm1_input.h").read_text(encoding="utf-8", errors="replace")
        m = re.search(r"static const int8_t FM1_KEYMAP.*?^\};", text, re.S | re.M)
        if not m:
            raise RuntimeError("FM1_KEYMAP not found in firmware/hal/fm1_input.h")
        (GEN / "sim_keymap.h").write_text(m.group(0) + "\n", encoding="utf-8")
        version = (ROOT / "VERSION.txt").read_text().strip() if (ROOT / "VERSION.txt").exists() else "DEV"
        cc = os.environ.get("CC", "cc").split()
        cmd = cc + ["-O2", "-Wno-unused-function", "-Wno-int-to-pointer-cast", "-Wno-macro-redefined",
                    "-w", f"-I{GEN}", "-Ifirmware/src", "-Itests", f'-DFELUCCA_VERSION="DRUM-{version}"']
        if native:
            cmd.append("-DGRIND_NATIVE=1")
        if os.name == "nt":
            cmd += ["-DSIGALRM=14", "-Dalarm(x)=0"]
        cmd += ["-o", str(EXE), "tests/sim_cli.c", "-lm"]
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError("build failed:\n" + (r.stderr or r.stdout)[-3000:])
        cls.native = native

    @classmethod
    def start(cls, native=None, rebuild=False):
        want = cls.native if native is None else native
        if cls.proc and cls.proc.poll() is None and not rebuild and want == cls.native:
            return
        cls.stop()
        if rebuild or not EXE.exists() or want != cls.native:
            cls.build(want)
        cls.proc = subprocess.Popen([str(EXE)], cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                    stderr=subprocess.DEVNULL, text=True, bufsize=1)

    @classmethod
    def stop(cls):
        if cls.proc and cls.proc.poll() is None:
            try:
                cls.proc.stdin.write("quit\n")
                cls.proc.stdin.flush()
                cls.proc.wait(timeout=3)
            except Exception:
                cls.proc.kill()
        cls.proc = None

    @classmethod
    def ask(cls, line):
        cls.start()
        try:
            cls.proc.stdin.write(line + "\n")
            cls.proc.stdin.flush()
            out = cls.proc.stdout.readline()
        except (BrokenPipeError, OSError):
            out = ""
        if not out:
            cls.proc = None
            raise RuntimeError("the simulator process ended; call sim_reset to start it again")
        return json.loads(out)


def png(width, height, rgb):
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    raw = b"".join(b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def read_ppm(path):
    data = Path(path).read_bytes()
    parts = data.split(b"\n", 3)
    w, h = (int(x) for x in parts[1].split())
    pix = parts[3]
    return w, h, pix


def scale(w, h, pix, k):
    if k <= 1:
        return w, h, pix
    rows = []
    for y in range(h):
        row = b"".join(pix[(y * w + x) * 3:(y * w + x) * 3 + 3] * k for x in range(w))
        rows.extend([row] * k)
    return w * k, h * k, b"".join(rows)


def need_ok(reply):
    if not reply.get("ok"):
        raise RuntimeError(reply.get("error", "failed"))
    return reply


def t_sim_reset(a):
    native = bool(a.get("grind_native", True))
    Sim.start(native=native, rebuild=bool(a.get("rebuild", False)))
    need_ok(Sim.ask(f"init {1 if a.get('fresh_flash', True) else 0}"))
    return text(json.dumps(Sim.ask("state")))


def t_sim_press(a):
    b = str(a["button"]).upper()
    if b not in BUTTONS:
        raise ValueError(f"button must be one of {BUTTONS}")
    if a.get("hold"):
        need_ok(Sim.ask(f"btn {b} 1"))
        need_ok(Sim.ask(f"run {int(a['hold'])}"))
        need_ok(Sim.ask(f"btn {b} 0"))
        need_ok(Sim.ask("run 2048"))
    else:
        need_ok(Sim.ask(f"tap {b}"))
    return text(json.dumps(Sim.ask("state")))


def t_sim_turn(a):
    e = str(a["encoder"]).upper()
    if e not in ENCODERS:
        raise ValueError(f"encoder must be one of {ENCODERS}")
    need_ok(Sim.ask(f"enc {e} {int(a['steps'])}"))
    return text(json.dumps(Sim.ask("state")))


def t_sim_key(a):
    k = int(a["key"])
    need_ok(Sim.ask(f"key {k} 1"))
    need_ok(Sim.ask(f"run {int(a.get('hold_frames', 2048))}"))
    need_ok(Sim.ask(f"key {k} 0"))
    need_ok(Sim.ask("run 2048"))
    return text(json.dumps(Sim.ask("state")))


def t_sim_run(a):
    need_ok(Sim.ask(f"run {max(0, int(a['frames']))}"))
    return text(json.dumps(Sim.ask("state")))


def t_sim_state(a):
    r = Sim.ask("state")
    r.update({k: v for k, v in need_ok(Sim.ask("leds")).items() if k != "ok"})
    return text(json.dumps(r))


def t_sim_audio(a):
    return text(json.dumps(need_ok(Sim.ask(f"audio {max(1, int(a.get('frames', 4096)))}"))))


def t_sim_screenshot(a):
    path = ROOT / "build" / "sim" / "host" / "screen.ppm"
    need_ok(Sim.ask(f"shot {path}"))
    w, h, pix = scale(*read_ppm(path), max(1, min(int(a.get("scale", 2)), 4)))
    return [{"type": "image", "data": base64.b64encode(png(w, h, pix)).decode(), "mimeType": "image/png"}]


# ---------------------------------------------------------------- emulator (fm1-emu)

def read_bmp(path):
    d = Path(path).read_bytes()
    off = struct.unpack("<I", d[10:14])[0]
    w, h = struct.unpack("<ii", d[18:26])
    bpp = struct.unpack("<H", d[28:30])[0]
    if bpp not in (24, 32):
        raise ValueError(f"unsupported BMP depth {bpp}")
    bp, rows = bpp // 8, []
    stride = (w * bp + 3) // 4 * 4
    for y in range(abs(h)):
        r = d[off + y * stride:off + y * stride + w * bp]
        rows.append(b"".join(bytes((r[x * bp + 2], r[x * bp + 1], r[x * bp])) for x in range(w)))
    if h > 0:
        rows.reverse()
    return w, abs(h), b"".join(rows)


def wav_stats(path):
    try:
        with wave.open(str(path), "rb") as w:
            n, ch, width = w.getnframes(), w.getnchannels(), w.getsampwidth()
            raw = w.readframes(n)
    except (wave.Error, EOFError, FileNotFoundError):
        return None
    if width != 2 or not raw:
        return {"frames": n, "note": f"{width * 8}-bit audio, level not computed"}
    vals = struct.unpack("<" + "h" * (len(raw) // 2), raw)
    peak = max(abs(v) for v in vals) / 32768.0
    rms = (sum(v * v for v in vals) / len(vals)) ** 0.5 / 32768.0
    return {"seconds": round(n / w.getframerate(), 2), "channels": ch, "peak": round(peak, 5), "rms": round(rms, 5)}


def t_emu_run(a):
    exe = os.environ.get("FM1_EMU")
    if not exe or not Path(exe).is_file():
        raise RuntimeError("set FM1_EMU to the fm1-emu binary (github.com/joakimeriksson/fm1-emu releases)")
    pkg = Path(a["package"]).expanduser()
    if not pkg.is_file() or pkg.suffix.lower() not in (".fwsc", ".bin"):
        raise ValueError("package must be an existing .fwsc (or app .bin) file")
    ms = max(100, min(int(a.get("ms", 1500)), 20000))
    snaps = sorted({int(x) for x in a.get("snapshots", [ms - 50])})
    cmd = [exe, "--bin" if pkg.suffix.lower() == ".bin" else "--fwsc", str(pkg.resolve()), "--ms", str(ms),
           "--steps", str(int(a.get("steps", 400_000_000))), "--snap", ",".join(str(x) for x in snaps), "--wav", "run.wav"]
    if a.get("keys"):
        cmd += ["--keys", str(a["keys"])]
    with tempfile.TemporaryDirectory(prefix="grind-emu-") as d:
        (Path(d) / "work").mkdir()
        try:
            r = subprocess.run(cmd, cwd=d, capture_output=True, text=True, encoding="utf-8", errors="replace",
                               timeout=int(a.get("timeout", 150)))
        except subprocess.TimeoutExpired:
            raise RuntimeError("the emulator run timed out; lower ms or steps (the batch runner interprets: about 17 s per emulated second)")
        out = r.stdout + r.stderr
        keep = [ln for ln in out.splitlines() if not re.match(r"\s+(r_|if_|0x0)", ln)]
        facts = {"exit": r.returncode}
        for key, rx in (("stopped", r"^stopped after (\d+) instructions \((.*?)[,)]"), ("emulated", r"^time (\d+) ms, (\d+) interrupts, (\d+) audio halves"),
                        ("lcd", r"^lcd frames=(\d+) inverted=(\w+) display_on=(\w+)"), ("unknown_mmio", r"^(\d+) unknown MMIO accesses")):
            for ln in keep:
                m = re.match(rx, ln)
                if m:
                    facts[key] = m.groups()
                    break
        facts["audio"] = wav_stats(Path(d) / "run.wav")
        content = [{"type": "text", "text": json.dumps(facts) + "\n" + "\n".join(keep[:40])}]
        for t in snaps:
            f = Path(d) / "work" / f"lcd_{t}.bmp"
            if f.is_file():
                w, h, pix = scale(*read_bmp(f), max(1, min(int(a.get("scale", 2)), 4)))
                content.append({"type": "text", "text": f"screen at {t} ms:"})
                content.append({"type": "image", "data": base64.b64encode(png(w, h, pix)).decode(), "mimeType": "image/png"})
        if r.returncode:
            content[0]["text"] = f"WARNING: the emulator exited with code {r.returncode}\n" + content[0]["text"]
        return content


def t_emu_fetch_package(a):
    tag = str(a["tag"])
    if not re.fullmatch(r"[A-Za-z0-9._-]+", tag):
        raise ValueError("tag has characters a release tag does not")
    dest = ROOT / "build" / "emu"
    dest.mkdir(parents=True, exist_ok=True)
    for pat in ("*.fwsc", "SHA256SUMS.txt"):
        r = subprocess.run(["gh", "release", "download", tag, "-R", a.get("repo", "DaFum/grind-machine"), "-p", pat, "--dir",
                            str(dest), "--clobber"], cwd=ROOT, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError((r.stderr or "gh failed").strip()[-500:])
    sums = {}
    for ln in (dest / "SHA256SUMS.txt").read_text().splitlines():
        parts = ln.split()
        if len(parts) == 2:
            sums[Path(parts[1].lstrip("*")).name] = parts[0]
    out = []
    for f in sorted(dest.glob("*.fwsc")):
        h = hashlib.sha256(f.read_bytes()).hexdigest()
        out.append(f"{f}  sha256 {h}  " + ("matches SHA256SUMS" if sums.get(f.name) == h else "NOT in / differs from SHA256SUMS"))
    return text("\n".join(out) or "no .fwsc in that release")


# ---------------------------------------------------------------- logs

def text(s):
    if len(s) > MAX_TEXT:
        s = s[:MAX_TEXT] + f"\n... cut ({len(s) - MAX_TEXT} more characters; narrow the request)"
    return [{"type": "text", "text": s}]


def lines_of(path):
    p = Path(path).expanduser()
    if not p.is_file():
        raise FileNotFoundError(f"not a file: {p}")
    with open(p, encoding="utf-8", errors="replace") as f:
        return [ln.rstrip("\n") for ln in f]


def parse(line):
    if line.startswith("{"):
        try:
            d = json.loads(line)
            return d if isinstance(d, dict) else None
        except ValueError:
            return None
    return None


def shorten(line, n=300):
    return line if len(line) <= n else line[:n] + f" ...[{len(line) - n} more]"


def t_log_summary(a):
    ls = lines_of(a["path"])
    out = [f"{a['path']}: {len(ls)} lines"]
    recs = [(parse(x), x) for x in ls]
    js = [d for d, _ in recs if d and "event" in d]
    if js:
        times = [d.get("time") for d in js if d.get("time")]
        out.append(f"JSON events: {len(js)} ({len(ls) - len(js)} other lines); {times[0]} .. {times[-1]}")
        ev = collections.Counter(d["event"] for d in js)
        out.append("top events: " + ", ".join(f"{k} x{v}" for k, v in ev.most_common(12)))
        src = collections.Counter(d.get("source") for d in js)
        out.append("sources: " + ", ".join(f"{k} x{v}" for k, v in src.items()))
        cmds = [d for d in js if d["event"] == "command.start"]
        for d in cmds[:30]:
            dd = d.get("data", {})
            out.append(f"  command #{dd.get('commandId')} {dd.get('arguments')} at seq {d.get('seq')}")
        bad = re.compile(r"error|fail|timeout|abort|exception|denied|refus|stall", re.I)
        hits = [(d, x) for d, x in recs if d and "event" in d and d["event"] not in QUIET and bad.search(x)]
        out.append(f"error-like events (not stderr traces): {len(hits)}")
        for d, x in hits[:15]:
            out.append(f"  seq {d.get('seq')} {d['event']}: {shorten(json.dumps(d.get('data')), 200)}")
        out.append("last events:")
        for d in js[-5:]:
            out.append(f"  seq {d.get('seq')} {d['event']}: {shorten(json.dumps(d.get('data')), 200)}")
    else:
        bad = re.compile(r"error|fail|timeout|abort|exception|crash|assert|FAIL", re.I)
        hits = [(i + 1, x) for i, x in enumerate(ls) if bad.search(x)]
        out.append(f"error-like lines: {len(hits)}")
        for i, x in hits[:25]:
            out.append(f"  {i}: {shorten(x, 200)}")
        out.append("first lines:")
        out += [f"  {shorten(x, 200)}" for x in ls[:3]]
        out.append("last lines:")
        out += [f"  {shorten(x, 200)}" for x in ls[-5:]]
    return text("\n".join(out))


def t_log_read(a):
    ls = lines_of(a["path"])
    count = max(1, min(int(a.get("count", 100)), 1000))
    if a.get("tail"):
        start = max(0, len(ls) - count)
    else:
        start = max(0, int(a.get("start", 1)) - 1)
    sel = ls[start:start + count]
    return text(f"lines {start + 1}..{start + len(sel)} of {len(ls)}\n" + "\n".join(f"{start + i + 1}: {shorten(x, 600)}"
                                                                                     for i, x in enumerate(sel)))


def t_log_search(a):
    ls = lines_of(a["path"])
    pat = re.compile(a["pattern"] if a.get("regex") else re.escape(a["pattern"]), 0 if a.get("case") else re.I)
    ctx = max(0, min(int(a.get("context", 0)), 10))
    limit = max(1, min(int(a.get("max", 40)), 300))
    event = a.get("event")
    shown, hits, last = [], 0, -1
    for i, x in enumerate(ls):
        if not pat.search(x):
            continue
        if event:
            d = parse(x)
            if not d or d.get("event") != event:
                continue
        hits += 1
        if hits > limit:
            continue
        for j in range(max(last + 1, i - ctx), min(len(ls), i + ctx + 1)):
            shown.append(f"{j + 1}{':' if j == i else '-'} {shorten(ls[j], 500)}")
        last = i + ctx
        if ctx:
            shown.append("--")
    return text(f"{hits} matching lines" + (f" (first {limit} shown)" if hits > limit else "") + "\n" + "\n".join(shown))


def t_ci_log(a):
    if not re.fullmatch(r"\d+", str(a["run_id"])):
        raise ValueError("run_id must be a number (gh run list)")
    cmd = ["gh", "run", "view", str(a["run_id"]), "--log-failed" if a.get("failed_only", True) else "--log"]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if r.returncode:
        raise RuntimeError((r.stderr or "gh failed").strip()[-800:])
    body = r.stdout.splitlines()
    pat = a.get("grep")
    if pat:
        rx = re.compile(pat, re.I)
        body = [x for x in body if rx.search(x)]
    tail = max(1, min(int(a.get("tail", 150)), 1000))
    return text(f"{len(body)} lines" + (f" matching {pat!r}" if pat else "") + f", last {min(tail, len(body))}:\n"
                + "\n".join(body[-tail:]))


# ---------------------------------------------------------------- MCP plumbing

def obj(props, required=()):
    return {"type": "object", "properties": props, "required": list(required)}


S = {"type": "string"}
I = {"type": "integer"}
B = {"type": "boolean"}

TOOLS = {
    "sim_reset": (t_sim_reset, "Power on a fresh simulated GRIND MACHINE (builds the native simulator on first use; "
                  "grind_native=false builds the stock Felucca path). Returns the state.",
                  obj({"grind_native": B, "fresh_flash": B, "rebuild": B})),
    "sim_press": (t_sim_press, "Press and release a panel button (acts in the frames after the press, as on the device). "
                  "hold = frames to hold it down for a long press. Returns the state.",
                  obj({"button": {"type": "string", "enum": BUTTONS}, "hold": I}, ["button"])),
    "sim_turn": (t_sim_turn, "Turn an encoder; positive steps are clockwise. K1..K4 are the knobs. Returns the state.",
                 obj({"encoder": {"type": "string", "enum": ENCODERS}, "steps": I}, ["encoder", "steps"])),
    "sim_key": (t_sim_key, "Play a note key 0 (F3) .. 26 (G5): down, hold, up.",
                obj({"key": I, "hold_frames": I}, ["key"])),
    "sim_run": (t_sim_run, "Advance the audio clock by this many sample frames (about 48000 a second; a UI frame every "
                "15 ms). Use it to let a song play or a queued change land on its boundary.", obj({"frames": I}, ["frames"])),
    "sim_state": (t_sim_state, "Page, mode, selected track, transport, the on-screen message, LED masks and (GRIND) the "
                  "controller's commit count and status.", obj({})),
    "sim_audio": (t_sim_audio, "Advance and report the peak and RMS of what the firmware played (0 = silence).",
                  obj({"frames": I})),
    "sim_screenshot": (t_sim_screenshot, "The 240x240 screen as a PNG (scale 1..4, default 2).", obj({"scale": I})),
    "emu_run": (t_emu_run, "Run a firmware package (.fwsc) in the fm1-emu emulator from power-on: keys is its --keys script "
                "(times in ms, e.g. '1000-1100:EDIT,1500:enc3:+2,2000:n7'), snapshots the times of screenshots. Returns "
                "facts (exit code, instructions, interrupts, LCD frames, audio level) and the screens. Needs FM1_EMU. "
                "About 17 s of real time per emulated second.",
                obj({"package": S, "ms": I, "keys": S, "snapshots": {"type": "array", "items": I}, "steps": I, "scale": I,
                     "timeout": I}, ["package"])),
    "emu_fetch_package": (t_emu_fetch_package, "Download the .fwsc of a release of this repository (gh) into build/emu and "
                          "check it against that release's SHA256SUMS.", obj({"tag": S, "repo": S}, ["tag"])),
    "log_summary": (t_log_summary, "Summarise a log file: for an M-UPGRADE .jsonl the time span, top events, the "
                    "commands run, error-like events and the last events; for text the error-like lines and its ends. "
                    "Read-only; does not touch any device.", obj({"path": S}, ["path"])),
    "log_read": (t_log_read, "Read lines of a log file: start (1-based) and count, or tail=true for the end.",
                 obj({"path": S, "start": I, "count": I, "tail": B}, ["path"])),
    "log_search": (t_log_search, "Search a log file (case-insensitive substring, or regex=true), with context lines. "
                   "event filters JSON logs by their event name (for example stderr, command.start).",
                   obj({"path": S, "pattern": S, "regex": B, "case": B, "context": I, "max": I, "event": S},
                       ["path", "pattern"])),
    "ci_log": (t_ci_log, "Fetch a GitHub Actions run's log with gh (failed steps by default), optionally filtered "
               "by a regex, and return its last lines. Read-only.",
               obj({"run_id": S, "failed_only": B, "grep": S, "tail": I}, ["run_id"])),
}


def handle(msg):
    method, mid = msg.get("method"), msg.get("id")
    if method == "initialize":
        return {"protocolVersion": msg.get("params", {}).get("protocolVersion", "2025-06-18"),
                "capabilities": {"tools": {}}, "serverInfo": {"name": "grind-dev", "version": "0.1.0"}}
    if method == "ping":
        return {}
    if method == "tools/list":
        return {"tools": [{"name": n, "description": d, "inputSchema": s} for n, (_, d, s) in TOOLS.items()]}
    if method == "tools/call":
        p = msg.get("params", {})
        name = p.get("name")
        if name not in TOOLS:
            return {"content": text(f"unknown tool {name}"), "isError": True}
        try:
            return {"content": TOOLS[name][0](p.get("arguments") or {}), "isError": False}
        except Exception as e:                     # (reported to the caller, never fatal to the server)
            return {"content": text(f"{type(e).__name__}: {e}"), "isError": True}
    if mid is None:
        return None                                # a notification: no reply
    raise KeyError(method)


def main():
    for raw in sys.stdin:
        raw = raw.strip()
        if not raw:
            continue
        try:
            msg = json.loads(raw)
        except ValueError:
            continue
        try:
            res = handle(msg)
        except KeyError as e:
            if msg.get("id") is not None:
                print(json.dumps({"jsonrpc": "2.0", "id": msg["id"], "error": {"code": -32601, "message": f"no method {e}"}}),
                      flush=True)
            continue
        if res is not None and msg.get("id") is not None:
            print(json.dumps({"jsonrpc": "2.0", "id": msg["id"], "result": res}), flush=True)
    Sim.stop()


if __name__ == "__main__":
    main()
