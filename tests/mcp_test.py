#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/mcp/grind_mcp.py over its real stdio protocol: the simulator tools drive the firmware's C, the emulator tool
runs a finished package, the log tools read files. Needs a C compiler (CC); builds build/sim/host/sim_cli on first use.
The emulator checks run only with FM1_EMU (the fm1-emu binary) and FM1_EMU_PACKAGE (a .fwsc) set."""
import base64
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
fails = 0


def check(what, ok):
    global fails
    print(("ok   " if ok else "FAIL ") + what)
    fails += not ok


class Client:
    def __init__(self, env=None):
        self.p = subprocess.Popen([sys.executable, str(ROOT / "tools/mcp/grind_mcp.py")], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1, cwd=ROOT, env=env)
        self.n = 0

    def rpc(self, method, params=None):
        self.n += 1
        self.p.stdin.write(json.dumps({"jsonrpc": "2.0", "id": self.n, "method": method, "params": params or {}}) + "\n")
        self.p.stdin.flush()
        r = json.loads(self.p.stdout.readline())
        assert r["id"] == self.n
        return r

    def call(self, name, **args):
        r = self.rpc("tools/call", {"name": name, "arguments": args})["result"]
        return r, r["content"][0].get("text", "")

    def state(self):
        return json.loads(self.call("sim_state")[1])

    def close(self):
        self.p.stdin.close()
        self.p.wait(timeout=10)


def main():
    c = Client()
    init = c.rpc("initialize", {"protocolVersion": "2025-06-18"})["result"]
    check("initialize names the server and offers tools", init["serverInfo"]["name"] == "grind-dev" and "tools" in init["capabilities"])
    names = {t["name"] for t in c.rpc("tools/list")["result"]["tools"]}
    want = {"sim_reset", "sim_press", "sim_turn", "sim_key", "sim_run", "sim_state", "sim_audio", "sim_screenshot",
            "emu_run", "emu_fetch_package", "log_summary", "log_read", "log_search", "ci_log"}
    check("tools/list has every tool", want <= names)
    check("an unknown method answers with an error, not silence", "error" in c.rpc("nope"))

    # simulator: record the value before the action and require it to change
    r, t = c.call("sim_reset")
    s0 = json.loads(t)
    check("sim_reset boots a GRIND build", not r["isError"] and s0["ok"] and s0["grind"] is True)
    shot0 = c.call("sim_screenshot", scale=1)[0]["content"][0]
    png0 = base64.b64decode(shot0["data"])
    check("sim_screenshot returns a PNG", shot0["type"] == "image" and png0[:8] == b"\x89PNG\r\n\x1a\n" and len(png0) > 1000)
    c.call("sim_press", button="EDIT")
    s1 = c.state()
    check("pressing EDIT changes the page", s1["page"] != s0["page"])
    png1 = base64.b64decode(c.call("sim_screenshot", scale=1)[0]["content"][0]["data"])
    check("and the screen", png1 != png0)
    frames0 = s1["ui_frames"]
    c.call("sim_run", frames=4096)
    check("sim_run advances the firmware's UI frames", c.state()["ui_frames"] > frames0)
    before = png1
    c.call("sim_turn", encoder="K1", steps=3)
    after = base64.b64decode(c.call("sim_screenshot", scale=1)[0]["content"][0]["data"])
    check("turning KNOB 1 changes the screen", after != before)
    c.call("sim_key", key=2)
    check("a note key selects track 2", c.state()["track"] == 1)
    a = json.loads(c.call("sim_audio", frames=2048)[1])
    check("sim_audio reports a level", a["ok"] and "peak" in a and a["rms"] >= 0)
    r, t = c.call("sim_press", button="NOPE")
    check("a bad button is an error result, and the server lives", r["isError"] and "button" in t and c.state()["ok"])
    c.call("sim_reset")
    check("sim_reset starts over (page is the boot page again)", c.state()["page"] == s0["page"])

    # emulator: needs FM1_EMU (the fm1-emu binary) and FM1_EMU_PACKAGE (a .fwsc); skipped without them
    emu, pkg = os.environ.get("FM1_EMU"), os.environ.get("FM1_EMU_PACKAGE")
    if emu and pkg:
        r = c.call("emu_run", package=pkg, ms=1500, snapshots=[900, 1450], keys="1000-1100:EDIT", timeout=170)[0]
        facts = json.loads(r["content"][0]["text"].split("\n")[0])
        imgs = [x for x in r["content"] if x["type"] == "image"]
        check("emu_run boots the package: exit 0, the LCD drew frames, audio came out",
              not r["isError"] and facts["exit"] == 0 and int(facts["lcd"][0]) > 10 and bool(facts["audio"])
              and facts["audio"]["seconds"] > 1)
        check("emu_run returns both snapshots as PNG", len(imgs) == 2 and base64.b64decode(imgs[0]["data"])[:4] == b"\x89PNG")
        check("the EDIT press changed the screen between the snapshots", len(imgs) == 2 and imgs[0]["data"] != imgs[1]["data"])
    else:
        print("skip  emu_run boot (set FM1_EMU and FM1_EMU_PACKAGE)")
    c.close()
    env = dict(os.environ)                            # (a server without FM1_EMU, whatever this shell has set)
    env.pop("FM1_EMU", None)
    c = Client(env)
    r, t = c.call("emu_run", package=str(ROOT / "README.md"))
    check("emu_run without FM1_EMU says what to set", r["isError"] and "FM1_EMU" in t)
    r, t = c.call("emu_fetch_package", tag="v1; rm -rf /")
    check("emu_fetch_package rejects a tag with odd characters", r["isError"] and "characters" in t)

    # review findings: the build mode survives a restart, negative durations are refused, releases do not mix
    c.close()
    c = Client()
    c.call("sim_reset", grind_native=False)
    check("sim_reset with grind_native=false builds the stock path", json.loads(c.call("sim_state")[1])["grind"] is False)
    c.close()
    c = Client()                                      # (a new server: it must not reuse the stock binary as GRIND)
    r, t = c.call("sim_reset")
    check("a new server's default sim_reset is a GRIND build even after a stock build", json.loads(t)["grind"] is True)
    r, t = c.call("sim_press", button="EDIT", hold=-5)
    check("a negative hold is refused, not run as billions of frames", r["isError"] and "hold" in t)
    r, t = c.call("sim_key", key=2, hold_frames=-1)
    check("a negative key hold is refused", r["isError"] and "hold" in t)
    r, t = c.call("sim_run", frames=-1)
    check("a negative frame count is refused", r["isError"] and "frames" in t)
    sys.path.insert(0, str(ROOT / "tools" / "mcp"))
    import shutil
    import grind_mcp
    real_run = grind_mcp.subprocess.run

    def fake_gh(cmd, **kw):
        if cmd[0] != "gh":
            return real_run(cmd, **kw)
        tag, dest, pat = cmd[3], Path(cmd[cmd.index("--dir") + 1]), cmd[cmd.index("-p") + 1]
        dest.mkdir(parents=True, exist_ok=True)
        if pat == "*.fwsc":
            (dest / f"{tag}.fwsc").write_bytes(tag.encode())
        else:
            import hashlib
            (dest / "SHA256SUMS.txt").write_text(hashlib.sha256(tag.encode()).hexdigest() + f" *{tag}.fwsc\n")
        return subprocess.CompletedProcess(cmd, 0, "", "")
    grind_mcp.subprocess.run = fake_gh
    try:
        first = grind_mcp.t_emu_fetch_package({"tag": "relA"})[0]["text"]
        second = grind_mcp.t_emu_fetch_package({"tag": "relB"})[0]["text"]
        again = grind_mcp.t_emu_fetch_package({"tag": "relA"})[0]["text"]
    finally:
        grind_mcp.subprocess.run = real_run
        shutil.rmtree(ROOT / "build" / "emu" / "relA", ignore_errors=True)
        shutil.rmtree(ROOT / "build" / "emu" / "relB", ignore_errors=True)
    check("emu_fetch_package: a second release does not list the first one's package",
          "relB.fwsc" in second and "relA.fwsc" not in second and "matches SHA256SUMS" in second)
    check("emu_fetch_package: the first release is still just its own", "relA.fwsc" in first and "relB.fwsc" not in again)

    # logs
    with tempfile.TemporaryDirectory() as d:
        j = Path(d) / "m-upgrade.jsonl"
        rows = [
            {"time": "2026-11-28T13:00:00Z", "seq": 1, "source": "app", "event": "session.start", "data": {"version": "3"}},
            {"time": "2026-11-28T13:00:01Z", "seq": 2, "source": "backend", "event": "command.start",
             "data": {"commandId": 1, "arguments": ["ks-scan"], "timeoutMs": 60000}},
            {"time": "2026-11-28T13:00:02Z", "seq": 3, "source": "backend", "event": "stderr",
             "data": {"line": "[ks-trace] ordinary trace line"}},
            {"time": "2026-11-28T13:00:03Z", "seq": 4, "source": "backend", "event": "ota.progress",
             "data": {"sent": 410112, "error": "KS MIDI receive timeout error=258"}},
            {"time": "2026-11-28T13:00:04Z", "seq": 5, "source": "app", "event": "session.end", "data": {}},
        ]
        j.write_text("\n".join(json.dumps(x) for x in rows) + "\n", encoding="utf-8")
        t = c.call("log_summary", path=str(j))[1]
        check("log_summary counts events, lists the command and finds the timeout",
              "5 lines" in t and "ks-scan" in t and "timeout" in t and "410112" in t)
        errs = t.split("error-like")[1].split("last events")[0]
        check("log_summary counts neither a stderr trace nor a command's timeoutMs field as an error",
              "ordinary trace line" not in errs and "timeoutMs" not in errs)
        t = c.call("log_search", path=str(j), pattern="timeout")[1]
        check("log_search finds the timeout lines", t.startswith("2 matching") and "410112" in t)
        t = c.call("log_search", path=str(j), pattern="line", event="stderr")[1]
        check("log_search filters by JSON event", t.startswith("1 matching") and "ordinary" in t)
        t = c.call("log_search", path=str(j), pattern="ota.progress", context=1)[1]
        check("log_search shows context lines", "ordinary trace line" in t and "session.end" in t)
        t = c.call("log_read", path=str(j), tail=True, count=2)[1]
        check("log_read tail gives the last two lines", "lines 4..5 of 5" in t)
        r, t = c.call("log_read", path=str(Path(d) / "missing.log"))
        check("a missing file is an error result", r["isError"] and "not a file" in t)
        txt = Path(d) / "serial.log"
        txt.write_text("12:00:00.1 boot\n12:00:01.2 CRASH at 0x1234\n12:00:02.3 ok\n", encoding="utf-8")
        check("log_summary reads plain text and flags the crash line", "CRASH" in c.call("log_summary", path=str(txt))[1])
    r, t = c.call("ci_log", run_id="12; rm -rf /")
    check("ci_log rejects a run id that is not a number", r["isError"] and "number" in t)
    c.close()
    print("mcp_test:", "all passed" if not fails else f"{fails} FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
