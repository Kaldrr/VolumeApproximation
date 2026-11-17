"""Run build/test commands without ever touching the MCP stdio channel."""

from __future__ import annotations

import asyncio
import os
import signal
import time
from collections.abc import Awaitable, Callable
from dataclasses import dataclass
from pathlib import Path


@dataclass
class ProcessResult:
    returncode: int | None  # None: stopped by us (timeout or cancellation)
    output: str
    duration_s: float
    timed_out: bool


async def run_process(
    args: list[str],
    *,
    cwd: Path,
    timeout_s: float,
    log_path: Path | None = None,
    log_mode: str = "w",
    env: dict[str, str] | None = None,
    on_line: Callable[[str], Awaitable[None]] | None = None,
) -> ProcessResult:
    """Run `args` (no shell), streaming stdout+stderr line by line.

    stdin is /dev/null: with the stdio transport the server's own stdin *is* the MCP
    protocol channel, and an inherited stdin would let a child read protocol messages.
    The child gets its own process group so a timeout or a cancelled tool call can stop
    the whole tree (cmake -> ninja -> compilers), not just the top process.
    """
    start = time.monotonic()
    proc = await asyncio.create_subprocess_exec(
        *args,
        cwd=cwd,
        env={**os.environ, **(env or {})},
        stdin=asyncio.subprocess.DEVNULL,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.STDOUT,
        start_new_session=True,
    )
    chunks: list[str] = []
    log = log_path.open(log_mode, encoding="utf-8") if log_path else None
    timed_out = False
    try:
        async with asyncio.timeout(timeout_s):
            assert proc.stdout is not None
            async for raw in proc.stdout:
                line = raw.decode("utf-8", errors="replace")
                chunks.append(line)
                if log:
                    log.write(line)
                if on_line:
                    await on_line(line)
            await proc.wait()
    except TimeoutError:
        timed_out = True
    finally:
        if proc.returncode is None:  # timeout, or the client cancelled the tool call
            await _stop(proc)
        if log:
            log.close()
    return ProcessResult(
        returncode=None if timed_out else proc.returncode,
        output="".join(chunks),
        duration_s=round(time.monotonic() - start, 2),
        timed_out=timed_out,
    )


async def _stop(proc: asyncio.subprocess.Process) -> None:
    for sig, wait_s in ((signal.SIGTERM, 5.0), (signal.SIGKILL, 5.0)):
        try:
            os.killpg(proc.pid, sig)
        except ProcessLookupError:
            return
        try:
            await asyncio.wait_for(proc.wait(), wait_s)
            return
        except TimeoutError:
            continue


async def version_line(args: list[str], cwd: Path, pick_last: bool = False) -> str | None:
    """First (or last) non-empty output line of a version command, None if it isn't installed."""
    try:
        result = await run_process(args, cwd=cwd, timeout_s=20)
    except FileNotFoundError:
        return None
    lines = [l.strip() for l in result.output.splitlines() if l.strip()]
    if result.returncode != 0 or not lines:
        return None
    return lines[-1] if pick_last else lines[0]
