"""End-to-end check: launch the server over stdio exactly like Claude Code does and call every tool.

    uv run python scripts/e2e.py [preset]      (default preset: cpu-debug)

Builds and tests the preset for real, so the first run can take minutes.
"""

import asyncio
import json
import sys
import time
from pathlib import Path

from mcp import Client, StdioServerParameters

SERVER_DIR = Path(__file__).resolve().parents[1]


def show(title: str, result) -> dict:
    data = result.structured_content
    if data is None:  # unstructured / error results
        data = {"text": " ".join(getattr(c, "text", "") for c in result.content)}
    data = data.get("result", data) if isinstance(data, dict) else data
    status = "ERROR" if result.is_error else "ok"
    text = json.dumps(data, indent=1)
    print(f"\n=== {title}: {status}\n{text[:1500]}{' ...' if len(text) > 1500 else ''}")
    return data


async def main(preset: str) -> int:
    params = StdioServerParameters(command="uv", args=["run", "--directory", str(SERVER_DIR),
                                                       "volume-approximation-mcp"])
    progress_events: list[tuple[float, float | None, str | None]] = []

    async def on_progress(progress: float, total: float | None, message: str | None) -> None:
        progress_events.append((progress, total, message))

    failures = 0
    async with Client(params) as client:
        tools = [t.name for t in (await client.list_tools()).tools]
        print("tools:", tools)

        show("environment", await client.call_tool("environment", {}))
        configs = show("list_configurations", await client.call_tool("list_configurations", {}))
        if preset not in {c["name"] for c in configs}:
            print(f"unknown preset {preset}")
            return 1

        start = time.monotonic()
        tests = show(f"run_tests({preset})", await client.call_tool(
            "run_tests", {"preset": preset}, read_timeout_seconds=3600, progress_callback=on_progress))
        print(f"  took {time.monotonic() - start:.1f}s, {len(progress_events)} progress notifications")
        failures += not tests.get("success", False)

        show("list_tests", await client.call_tool("list_tests", {"preset": preset}))
        show("get_diagnostics(warning)", await client.call_tool(
            "get_diagnostics", {"preset": preset, "severity": "warning"}))
        smoke = show("smoke_run", await client.call_tool("smoke_run", {"preset": preset, "seconds": 5}))
        failures += not smoke.get("ok", False)

        log = await client.read_resource(f"build://{preset}/test-log")
        text = log.contents[0].text
        print(f"\n=== resource build://{preset}/test-log: {len(text)} chars, last line: {text.splitlines()[-1]!r}")

        bad = await client.call_tool("build", {"preset": "cpu-debug; touch /tmp/pwned"})
        print(f"\n=== injection attempt rejected: {bad.is_error} ->", bad.content[0].text[:120])
        failures += not bad.is_error

    print(f"\nE2E {'PASSED' if failures == 0 else f'FAILED ({failures})'}")
    return failures


if __name__ == "__main__":
    sys.exit(asyncio.run(main(sys.argv[1] if len(sys.argv) > 1 else "cpu-debug")))
