"""Turn build, test and run output into structured findings."""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from pathlib import Path

from .models import Diagnostic, SanitizerFinding, TestCase

_ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
# project_options passes -fdiagnostics-color=always, so compiler output is coloured
strip_ansi = lambda text: _ANSI_RE.sub("", text)  # noqa: E731

_GCC_RE = re.compile(
    r"^(?P<file>[^\s:][^:]*?):(?P<line>\d+):(?:(?P<col>\d+):)? "
    r"(?P<sev>fatal error|error|warning|note): (?P<msg>.*)$"
)
_NVCC_RE = re.compile(
    r"^(?P<file>[^\s(][^(]*?)\((?P<line>\d+)\): (?P<sev>error|warning)(?: #[\w-]+)?: (?P<msg>.*)$"
)
_CMAKE_RE = re.compile(
    r"^CMake (?P<sev>Error|Warning)(?: \(dev\))?(?: at (?P<file>[^:]+):(?P<line>\d+)(?: \([^)]*\))?)?:\s*(?P<msg>.*)$"
)
_LINKER_RE = re.compile(r"(undefined reference to|multiple definition of|cannot find -l|relocation .* against)")
_CONTEXT_RE = re.compile(r"^\s*\d*\s\|")  # GCC's "   43 | code" and "      |   ^~~" lines
_NINJA_PROGRESS_RE = re.compile(r"^\[(\d+)/(\d+)\]\s+(.*)$")


def _relative(path: str, root: Path) -> tuple[str, bool]:
    """(display path, in_project). Generated files under build/ and system headers are not 'project'."""
    p = Path(path)
    if not p.is_absolute():
        p = (root / p).resolve()
    try:
        rel = p.relative_to(root)
    except ValueError:
        return path, False
    return str(rel), not (rel.parts and rel.parts[0].startswith("build"))


def parse_diagnostics(output: str, root: Path) -> list[Diagnostic]:
    lines = strip_ansi(output).splitlines()
    found: list[Diagnostic] = []
    seen: set[tuple] = set()
    last: Diagnostic | None = None

    def add(diag: Diagnostic) -> None:
        nonlocal last
        key = (diag.file, diag.line, diag.column, diag.severity, diag.message)
        if key in seen:
            last = None
            return
        seen.add(key)
        found.append(diag)
        last = diag

    i = 0
    while i < len(lines):
        line = lines[i]
        if m := _GCC_RE.match(line):
            file, in_project = _relative(m["file"], root)
            sev = "error" if m["sev"] in ("error", "fatal error") else m["sev"]
            if sev == "note":
                if last is not None and len(last.notes) < 3:
                    last.notes.append(f"{file}:{m['line']}: {m['msg']}"[:300])
            else:
                add(Diagnostic(severity=sev, message=m["msg"][:500], file=file, line=int(m["line"]),
                               column=int(m["col"]) if m["col"] else None, in_project=in_project))
                # GCC follows with the source line and the caret line
                while last is not None and i + 1 < len(lines) and _CONTEXT_RE.match(lines[i + 1]) and len(last.context) < 2:
                    i += 1
                    last.context.append(lines[i].rstrip()[:300])
        elif m := _NVCC_RE.match(line):
            file, in_project = _relative(m["file"], root)
            add(Diagnostic(severity=m["sev"], message=m["msg"][:500], file=file, line=int(m["line"]),
                           source="nvcc", in_project=in_project))
        elif m := _CMAKE_RE.match(line):
            # The message continues on the following indented lines, up to a blank line
            msg_lines = [m["msg"]] if m["msg"] else []
            while i + 1 < len(lines) and (lines[i + 1].startswith("  ") or not msg_lines and not lines[i + 1].strip()):
                i += 1
                if lines[i].strip():
                    msg_lines.append(lines[i].strip())
            file, in_project = (_relative(m["file"], root) if m["file"] else (None, True))
            add(Diagnostic(severity="error" if m["sev"] == "Error" else "warning",
                           message=" ".join(msg_lines)[:500] or "CMake " + m["sev"], file=file,
                           line=int(m["line"]) if m["line"] else None, source="cmake", in_project=in_project))
        elif _LINKER_RE.search(line):
            add(Diagnostic(severity="error", message=line.strip()[:500], source="linker", in_project=True))
        i += 1
    return found


def ninja_progress(line: str) -> tuple[int, int, str] | None:
    if m := _NINJA_PROGRESS_RE.match(strip_ansi(line)):
        return int(m[1]), int(m[2]), m[3]
    return None


# --- tests -------------------------------------------------------------------------

_INTERESTING_TEST_LINE = re.compile(
    r"(Failure|Expected|Which is|Value of|Actual|  Actual|error:|runtime error|"
    r"ERROR: (Address|Leak)Sanitizer|WARNING: ThreadSanitizer|SUMMARY:|\[  FAILED  \])"
)


def test_output_excerpt(output: str, tail_lines: int = 15, limit: int = 3000) -> str:
    lines = strip_ansi(output).splitlines()
    picked = [l for l in lines if _INTERESTING_TEST_LINE.search(l)][:20]
    tail = lines[-tail_lines:]
    text = "\n".join(picked + (["--- output tail ---"] if picked else []) + tail)
    return text if len(text) <= limit else text[: limit - 15] + "\n...[truncated]"


def parse_junit(xml_text: str) -> list[TestCase]:
    """CTest's --output-junit format: <testcase status="run|fail|disabled|notrun">."""
    root = ET.fromstring(xml_text)
    cases: list[TestCase] = []
    for tc in root.iter("testcase"):
        name = tc.get("name", "?")
        duration = float(tc.get("time") or 0.0)
        status_attr = tc.get("status", "run")
        out = "".join(e.text or "" for e in tc.iter("system-out"))
        failure = tc.find("failure")
        skipped = tc.find("skipped")
        if failure is not None or status_attr == "fail":
            reason = (failure.get("message") if failure is not None else None) or "failed"
            cases.append(TestCase(name=name, status="failed", duration_s=duration, reason=reason,
                                  output_excerpt=test_output_excerpt(out)))
        elif status_attr == "notrun":
            cases.append(TestCase(name=name, status="failed", reason="not run (test executable missing?)",
                                  output_excerpt=test_output_excerpt(out)))
        elif skipped is not None or status_attr == "disabled":
            reason = skipped.get("message") if skipped is not None else "disabled"
            cases.append(TestCase(name=name, status="skipped", duration_s=duration, reason=reason))
        else:
            cases.append(TestCase(name=name, status="passed", duration_s=duration))
    return cases


# --- sanitizers --------------------------------------------------------------------

_SAN_HEADER_RE = re.compile(
    r"(?:==\d+==ERROR: (?P<asan>(?:Address|Leak)Sanitizer: [^\n]*?)(?: on address.*)?$)"
    r"|(?:WARNING: ThreadSanitizer: (?P<tsan>.+?) \(pid=\d+\))"
    r"|(?:(?P<ubloc>\S+:\d+:\d+): runtime error: (?P<ubmsg>.*))"
)
_FRAME_RE = re.compile(r"#\d+ 0x[0-9a-f]+ in (?P<func>.+?) (?P<loc>/\S+:\d+)")


def parse_sanitizer_reports(output: str, root: Path) -> list[SanitizerFinding]:
    lines = strip_ansi(output).splitlines()
    findings: list[SanitizerFinding] = []
    for idx, line in enumerate(lines):
        m = _SAN_HEADER_RE.search(line)
        if not m:
            continue
        if m["ubloc"]:
            loc, _ = _relative(m["ubloc"], root)
            findings.append(SanitizerFinding(kind="UndefinedBehaviorSanitizer", first_project_frame=loc,
                                             summary=m["ubmsg"][:300]))
            continue
        kind = m["asan"] or f"ThreadSanitizer: {m['tsan']}"
        frame = summary = None
        for follow in lines[idx + 1 : idx + 80]:
            if summary is None and follow.startswith("SUMMARY:"):
                summary = follow[:300]
            if frame is None and (fm := _FRAME_RE.search(follow)):
                loc, in_project = _relative(fm["loc"], root)
                if in_project and not loc.startswith("/"):
                    frame = f"{fm['func'][:120]} at {loc}"
            if _SAN_HEADER_RE.search(follow):
                break
        findings.append(SanitizerFinding(kind=kind.strip(), first_project_frame=frame, summary=summary))
    return findings[:20]
