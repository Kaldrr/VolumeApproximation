"""MCP server exposing VolumeApproximation's presets, builds, tests and smoke runs.

Design rules (see README.md):
* Only presets from CMakePresets.json; never arbitrary commands or CMake arguments.
* Subprocesses get argument lists (no shell) and /dev/null as stdin.
* Results are summaries and parsed findings; full logs are available as resources.
* One build/test/run at a time: each one already uses every core.
"""

import asyncio
import json
import os
import re
import shutil
import time
from pathlib import Path
from typing import Literal

from mcp.server.mcpserver import Context, MCPServer
from mcp.server.mcpserver.exceptions import ToolError
from mcp_types import ToolAnnotations

from . import parsing
from .models import (
    BuildResult,
    Diagnostic,
    EnvironmentInfo,
    PresetInfo,
    SmokeRunResult,
    TestInfo,
    TestRunResult,
)
from .project import (
    Preset,
    ProjectError,
    find_repo_root,
    load_presets,
    require_preset,
    require_target,
    require_test_regex,
)
from .runner import run_process, version_line

ROOT = find_repo_root()
BUILD_TIMEOUT_S = 45 * 60  # a cold CUDA build with LTO takes several minutes
TEST_TIMEOUT_S = 20 * 60
MAX_DIAGNOSTICS = 40
MAX_LISTED_TESTS = 20
LOG_RESOURCE_LIMIT = 200_000  # characters; logs are tailed beyond this
PROGRESS_INTERVAL_S = 0.5

_job_lock = asyncio.Lock()

mcp = MCPServer(
    name="volume-approximation",
    instructions=(
        "Build, test and smoke-run the VolumeApproximation C++/CUDA/Qt project. "
        "Configurations are the CMake presets (list_configurations). The CPU-only presets must always "
        "build; CUDA presets need nvcc. run_tests rebuilds first, so call it directly to verify a change. "
        "Results are summaries; read build://<preset>/build-log or test-log for full output."
    ),
)


# --- helpers ------------------------------------------------------------------------


def _preset(name: str) -> Preset:
    try:
        return require_preset(load_presets(ROOT), name)
    except ProjectError as e:
        raise ToolError(str(e)) from e


def _nvcc() -> str | None:
    if found := shutil.which("nvcc"):
        return found
    for candidate in (os.environ.get("CUDACXX"), "/opt/cuda/bin/nvcc", "/usr/local/cuda/bin/nvcc"):
        if candidate and Path(candidate).is_file():
            return candidate
    return None


def _require_usable(preset: Preset) -> None:
    if preset.cuda_enabled and _nvcc() is None:
        raise ToolError(f"Preset {preset.name!r} needs the CUDA toolkit (nvcc), which isn't installed here. "
                        "Use a cpu-* preset.")


def _state_file(preset: Preset) -> Path:
    return preset.state_dir / "last_build.json"


def _save_build(preset: Preset, result: BuildResult, all_diagnostics: list[Diagnostic]) -> None:
    preset.state_dir.mkdir(parents=True, exist_ok=True)
    state = {
        "success": result.success,
        "finished_at": time.time(),
        "diagnostics": [d.model_dump() for d in all_diagnostics],
    }
    _state_file(preset).write_text(json.dumps(state), encoding="utf-8")


def _last_build(preset: Preset) -> dict | None:
    try:
        return json.loads(_state_file(preset).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def _summarise(diagnostics: list[Diagnostic]) -> tuple[list[Diagnostic], int, int, bool]:
    errors = [d for d in diagnostics if d.severity == "error"]
    warnings = [d for d in diagnostics if d.severity == "warning" and d.in_project]
    errors.sort(key=lambda d: not d.in_project)  # project errors first
    picked = (errors + warnings)[:MAX_DIAGNOSTICS]
    return picked, len(errors), len(warnings), len(errors) + len(warnings) > len(picked)


def _progress_reporter(ctx: Context):
    last = 0.0

    async def on_line(line: str) -> None:
        nonlocal last
        progress = parsing.ninja_progress(line)
        now = time.monotonic()
        if progress and now - last >= PROGRESS_INTERVAL_S:
            last = now
            done, total, what = progress
            try:
                await ctx.report_progress(done, total, f"[{done}/{total}] {what[:100]}")
            except Exception:  # progress is best-effort, never fail a build over it
                pass

    return on_line


async def _build(preset: Preset, ctx: Context, target: str | None = None, fresh: bool = False,
                 configure_only: bool = False) -> BuildResult:
    """Configure if needed, then build. Caller holds _job_lock."""
    preset.state_dir.mkdir(parents=True, exist_ok=True)
    log = preset.state_dir / "build.log"
    log_uri = f"build://{preset.name}/build-log"
    output = ""
    duration = 0.0

    def finish(phase, success, timed_out=False) -> BuildResult:
        diagnostics = parsing.parse_diagnostics(output, ROOT)
        if not success and not any(d.severity == "error" for d in diagnostics):
            tail = "\n".join(parsing.strip_ansi(output).splitlines()[-15:])
            reason = "Timed out" if timed_out else "Failed without a recognised diagnostic"
            diagnostics.insert(0, Diagnostic(severity="error", message=f"{reason}. Output tail:\n{tail}"[:2000],
                                             source="cmake" if phase == "configure" else "compiler"))
        picked, n_err, n_warn, truncated = _summarise(diagnostics)
        result = BuildResult(preset=preset.name, success=success, phase=phase, duration_s=round(duration, 2),
                             timed_out=timed_out, error_count=n_err, warning_count=n_warn, diagnostics=picked,
                             truncated=truncated, log_resource=log_uri)
        _save_build(preset, result, diagnostics)
        return result

    if fresh or configure_only or not (preset.binary_dir / "build.ninja").is_file():
        await ctx.report_progress(0, None, f"Configuring {preset.name}")
        args = ["cmake", "--preset", preset.name] + (["--fresh"] if fresh else [])
        res = await run_process(args, cwd=ROOT, timeout_s=BUILD_TIMEOUT_S, log_path=log)
        output, duration = res.output, res.duration_s
        if res.returncode != 0 or configure_only:
            return finish("configure", res.returncode == 0, res.timed_out)
        log_mode = "a"
    else:
        log_mode = "w"

    args = ["cmake", "--build", "--preset", preset.name]
    if target:
        args += ["--target", require_target(target)]
    res = await run_process(args, cwd=ROOT, timeout_s=BUILD_TIMEOUT_S, log_path=log, log_mode=log_mode,
                            on_line=_progress_reporter(ctx))
    output += res.output
    duration += res.duration_s
    return finish("build", res.returncode == 0, res.timed_out)


def _read_log(preset_name: str, filename: str) -> str:
    try:
        preset = require_preset(load_presets(ROOT), preset_name)
    except ProjectError as e:
        raise ValueError(str(e)) from e
    path = preset.state_dir / filename
    if not path.is_file():
        return f"No {filename} for {preset_name} yet: it is written by this server's tools."
    text = parsing.strip_ansi(path.read_text(encoding="utf-8", errors="replace"))
    if len(text) > LOG_RESOURCE_LIMIT:
        text = f"[showing the last {LOG_RESOURCE_LIMIT} of {len(text)} characters]\n" + text[-LOG_RESOURCE_LIMIT:]
    return text


# --- tools --------------------------------------------------------------------------


@mcp.tool(annotations=ToolAnnotations(read_only_hint=True))
def list_configurations() -> list[PresetInfo]:
    """List the build configurations (CMake presets): build type, CUDA on/off, sanitizers,
    build directory, whether it is configured, and the last build result from this server.
    CUDA presets are marked unusable when nvcc isn't installed."""
    has_nvcc = _nvcc() is not None
    infos = []
    for p in load_presets(ROOT).values():
        last = _last_build(p)
        infos.append(PresetInfo(
            name=p.name, display_name=p.display_name, build_type=p.build_type, cuda_enabled=p.cuda_enabled,
            sanitizers=list(p.sanitizers), build_dir=str(p.binary_dir.relative_to(ROOT)),
            configured=(p.binary_dir / "build.ninja").is_file(), usable_here=has_nvcc or not p.cuda_enabled,
            last_build=None if last is None else ("success" if last["success"] else "failed"),
        ))
    return infos


@mcp.tool(annotations=ToolAnnotations(read_only_hint=False, destructive_hint=False, idempotent_hint=True))
async def configure(preset: str, ctx: Context, fresh: bool = False) -> BuildResult:
    """Run the CMake configure step for a preset. fresh=True discards the CMake cache first
    (`--fresh`), e.g. after changing options. build/run_tests configure automatically when needed,
    so this is mainly for reconfiguring."""
    p = _preset(preset)
    _require_usable(p)
    async with _job_lock:
        return await _build(p, ctx, fresh=fresh, configure_only=True)


@mcp.tool(annotations=ToolAnnotations(read_only_hint=False, destructive_hint=False, idempotent_hint=True))
async def build(preset: str, ctx: Context, target: str | None = None) -> BuildResult:
    """Build a preset (configuring first if needed), optionally a single CMake target such as
    'ApproximationLib', 'VolumeApproximation' or 'ApproximationLibTests'. Returns parsed errors and
    project warnings (file, line, message, source context), not the raw log. Reports progress."""
    p = _preset(preset)
    _require_usable(p)
    async with _job_lock:
        return await _build(p, ctx, target=target)


@mcp.tool(annotations=ToolAnnotations(read_only_hint=True))
def get_diagnostics(preset: str, severity: Literal["error", "warning", "all"] = "all",
                    file: str | None = None, include_generated: bool = False) -> list[Diagnostic]:
    """Diagnostics from the last build of a preset, filtered by severity and/or a file path
    substring. Generated code (build/) and system headers are excluded unless include_generated."""
    p = _preset(preset)
    last = _last_build(p)
    if last is None:
        raise ToolError(f"{preset} hasn't been built by this server yet; call build first.")
    diags = [Diagnostic.model_validate(d) for d in last["diagnostics"]]
    return [
        d for d in diags
        if (severity == "all" or d.severity == severity)
        and (include_generated or d.in_project)
        and (file is None or (d.file is not None and file in d.file))
    ][:200]


@mcp.tool(annotations=ToolAnnotations(read_only_hint=True))
async def list_tests(preset: str) -> list[TestInfo]:
    """List the CTest tests of a built preset (GoogleTest cases, discovered when ctest runs).
    Build the preset first if it isn't built."""
    p = _preset(preset)
    if not (p.binary_dir / "build.ninja").is_file():
        raise ToolError(f"{preset} isn't configured; call build first.")
    res = await run_process(["ctest", "--preset", preset, "--show-only=json-v1"], cwd=ROOT, timeout_s=120)
    try:
        data = json.loads(res.output[res.output.find("{"):])
    except ValueError:
        tail = "\n".join(res.output.splitlines()[-10:])
        raise ToolError(f"ctest couldn't list tests (is the test target built?):\n{tail}") from None
    tests = []
    for t in data.get("tests", []):
        labels = next((prop["value"] for prop in t.get("properties", []) if prop.get("name") == "LABELS"), [])
        tests.append(TestInfo(name=t["name"], labels=labels))
    return tests


@mcp.tool(annotations=ToolAnnotations(read_only_hint=False, destructive_hint=False, idempotent_hint=True))
async def run_tests(preset: str, ctx: Context, name_regex: str | None = None,
                    build_first: bool = True) -> TestRunResult:
    """Build the preset (unless build_first=False) and run its CTest tests, optionally only those
    whose name matches name_regex (ctest -R). Returns pass/fail/skip counts, and for failures the
    assertion and sanitizer lines plus an output tail. Debug presets run under ASan+UBSan
    (cuda-tsan: TSan), so sanitizer reports fail tests. CUDA tests skip without a GPU."""
    p = _preset(preset)
    _require_usable(p)
    if name_regex is not None:
        try:
            require_test_regex(name_regex)
        except ProjectError as e:
            raise ToolError(str(e)) from e
    log_uri = f"build://{preset}/test-log"
    async with _job_lock:
        build_result = None
        if build_first:
            build_result = await _build(p, ctx)
            if not build_result.success:
                return TestRunResult(preset=preset, success=False, build=build_result, log_resource=log_uri,
                                     message="Build failed, tests were not run. See build.diagnostics.")
        junit = p.state_dir / "junit.xml"
        junit.unlink(missing_ok=True)
        args = ["ctest", "--preset", preset, "--output-junit", str(junit)]
        if name_regex:
            args += ["-R", name_regex]
        test_line = re.compile(r"^\s*(\d+)/(\d+) Test\s+#\d+: (\S+)")
        last = 0.0

        async def on_line(line: str) -> None:
            nonlocal last
            if (m := test_line.match(line)) and time.monotonic() - last >= PROGRESS_INTERVAL_S:
                last = time.monotonic()
                try:
                    await ctx.report_progress(int(m[1]), int(m[2]), m[3])
                except Exception:
                    pass

        res = await run_process(args, cwd=ROOT, timeout_s=TEST_TIMEOUT_S, log_path=p.state_dir / "test.log",
                                on_line=on_line)

    if not junit.is_file():
        tail = "\n".join(parsing.strip_ansi(res.output).splitlines()[-15:])
        return TestRunResult(preset=preset, success=False, build=build_result, duration_s=res.duration_s,
                             log_resource=log_uri, message=f"ctest produced no results:\n{tail}")
    cases = parsing.parse_junit(junit.read_text(encoding="utf-8", errors="replace"))
    failures = [c for c in cases if c.status == "failed"]
    skipped = [c for c in cases if c.status == "skipped"]
    message = None
    if res.timed_out:
        message = f"Timed out after {TEST_TIMEOUT_S} s"
    elif not cases:
        message = "No tests matched." if name_regex else "No tests found."
    return TestRunResult(
        preset=preset, success=res.returncode == 0 and not failures and bool(cases), build=build_result,
        total=len(cases), passed=sum(c.status == "passed" for c in cases), failed=len(failures),
        skipped=len(skipped), duration_s=res.duration_s, failures=failures[:MAX_LISTED_TESTS],
        skipped_tests=skipped[:MAX_LISTED_TESTS], message=message, log_resource=log_uri,
    )


@mcp.tool(annotations=ToolAnnotations(read_only_hint=True, idempotent_hint=True))
async def smoke_run(preset: str, seconds: int = 8) -> SmokeRunResult:
    """Start the built app headless (QT_QPA_PLATFORM=offscreen) for a few seconds (3-60) and report
    sanitizer findings and compute engines reported unavailable (e.g. CUDA without a GPU).
    The app is stopped at the time limit, which is the normal outcome; LeakSanitizer leak checks
    don't run in that case (they need a normal exit, see run_tests). No mesh is loaded."""
    p = _preset(preset)
    exe = p.binary_dir / "UserInterface" / "VolumeApproximation"
    if not exe.is_file():
        raise ToolError(f"{exe.relative_to(ROOT)} doesn't exist; build {preset} first.")
    seconds = max(3, min(60, seconds))
    async with _job_lock:
        res = await run_process([str(exe)], cwd=ROOT, timeout_s=seconds, env={"QT_QPA_PLATFORM": "offscreen"},
                                log_path=p.state_dir / "smoke.log")
    clean = parsing.strip_ansi(res.output)
    findings = parsing.parse_sanitizer_reports(clean, ROOT)
    skipped = re.findall(r"Compute engine '(.+?)' is not available", clean)
    ran_normally = res.timed_out or res.returncode == 0
    return SmokeRunResult(
        preset=preset, ok=ran_normally and not findings, ran_for_s=res.duration_s,
        exit_code=res.returncode, engines_skipped=skipped, sanitizer_findings=findings,
        output_tail="\n".join(clean.splitlines()[-15:])[-2000:], log_resource=f"build://{preset}/smoke-log",
    )


@mcp.tool(annotations=ToolAnnotations(read_only_hint=True))
async def environment() -> EnvironmentInfo:
    """Toolchain and machine facts: CMake, Ninja, C++ compiler, nvcc and clang-format versions,
    visible NVIDIA GPUs, and whether the project_options submodule is initialised."""
    nvcc = _nvcc()
    jobs = {
        "cmake": version_line(["cmake", "--version"], ROOT),
        "ninja": version_line(["ninja", "--version"], ROOT),
        "c++": version_line(["c++", "--version"], ROOT),
        "nvcc": version_line([nvcc, "--version"], ROOT, pick_last=True) if nvcc else None,
        "clang-format": version_line(["clang-format", "--version"], ROOT),
    }
    names = [k for k, v in jobs.items() if v is not None]
    values = await asyncio.gather(*(jobs[k] for k in names))
    tools: dict[str, str | None] = {k: None for k in jobs} | dict(zip(names, values))

    gpus: list[str] = []
    notes: list[str] = []
    if shutil.which("nvidia-smi"):
        res = await run_process(["nvidia-smi", "-L"], cwd=ROOT, timeout_s=20)
        gpus = [l.strip() for l in res.output.splitlines() if l.startswith("GPU")]
    if nvcc and not gpus:
        notes.append("nvcc is installed but no GPU is visible: CUDA builds work, CUDA tests will skip.")
    sub = await run_process(["git", "submodule", "status", "project_options"], cwd=ROOT, timeout_s=20)
    status = sub.output.strip() or "unknown"
    if status.startswith("-"):
        notes.append("project_options submodule not initialised: run `git submodule update --init`.")
    return EnvironmentInfo(repo_root=str(ROOT), tools=tools, gpus=gpus, project_options_submodule=status,
                           notes=notes)


# --- resources ----------------------------------------------------------------------


@mcp.resource("build://{preset}/build-log", mime_type="text/plain")
def build_log(preset: str) -> str:
    """Full configure + build output of the last build of a preset (tailed if very long)."""
    return _read_log(preset, "build.log")


@mcp.resource("build://{preset}/test-log", mime_type="text/plain")
def test_log(preset: str) -> str:
    """Full ctest output of the last run_tests call for a preset."""
    return _read_log(preset, "test.log")


@mcp.resource("build://{preset}/smoke-log", mime_type="text/plain")
def smoke_log(preset: str) -> str:
    """Full app output of the last smoke_run for a preset."""
    return _read_log(preset, "smoke.log")


# --- prompts ------------------------------------------------------------------------


@mcp.prompt()
def fix_build(preset: str = "cpu-debug") -> str:
    """Build a preset and fix whatever breaks."""
    return (
        f"Build the `{preset}` preset with the volume-approximation `build` tool. For each error, read the "
        "source around it and fix the root cause (not just the first symptom), then build again until it "
        "succeeds. Respect CLAUDE.md's hard requirements: CUDA code stays behind CUDA_ENABLED, and the "
        "CPU-only presets must keep building. Finish with `run_tests` on the same preset."
    )


@mcp.prompt()
def verify_change() -> str:
    """Verify the current changes across CPU and CUDA configurations."""
    return (
        "Verify the current working-tree changes with the volume-approximation tools: "
        "1) `run_tests` on cpu-debug; 2) `run_tests` on cuda-debug if `list_configurations` marks it usable; "
        "3) `smoke_run` on the presets you built. Report a table of preset / build / tests / smoke run, "
        "the first error of each failure, and what wasn't covered (e.g. no mesh was loaded in the smoke run)."
    )


def main() -> None:
    mcp.run("stdio")
