"""Structured tool results. Small on purpose: summaries and parsed findings, never raw logs."""

from __future__ import annotations

from typing import Literal

from pydantic import BaseModel, Field

Severity = Literal["error", "warning", "note"]


class Diagnostic(BaseModel):
    severity: Severity
    message: str
    file: str | None = Field(None, description="Relative to the repo root when inside it")
    line: int | None = None
    column: int | None = None
    source: Literal["compiler", "nvcc", "cmake", "linker"] = "compiler"
    in_project: bool = Field(True, description="False for generated code (build/) and system headers")
    context: list[str] = Field(default_factory=list, description="Source line and caret, as printed")
    notes: list[str] = Field(default_factory=list)


class PresetInfo(BaseModel):
    name: str
    display_name: str
    build_type: str
    cuda_enabled: bool
    sanitizers: list[str]
    build_dir: str
    configured: bool
    usable_here: bool = Field(description="False for CUDA presets when nvcc is not installed")
    last_build: str | None = Field(None, description="'success' / 'failed' / None if never built by this server")


class BuildResult(BaseModel):
    preset: str
    success: bool
    phase: Literal["configure", "build"]
    duration_s: float
    timed_out: bool = False
    error_count: int
    warning_count: int = Field(description="Warnings in project sources (generated code and system headers excluded)")
    diagnostics: list[Diagnostic] = Field(description="Errors first, then project warnings; capped")
    truncated: bool = False
    log_resource: str


class TestCase(BaseModel):
    name: str
    status: Literal["passed", "failed", "skipped"]
    duration_s: float = 0.0
    reason: str | None = None
    output_excerpt: str | None = Field(None, description="Assertion and sanitizer lines plus the output tail (failures only)")


class TestRunResult(BaseModel):
    preset: str
    success: bool
    build: BuildResult | None = Field(None, description="Present when the tests were (re)built first")
    total: int = 0
    passed: int = 0
    failed: int = 0
    skipped: int = 0
    duration_s: float = 0.0
    failures: list[TestCase] = Field(default_factory=list)
    skipped_tests: list[TestCase] = Field(default_factory=list)
    message: str | None = None
    log_resource: str


class SanitizerFinding(BaseModel):
    kind: str = Field(description="e.g. 'AddressSanitizer: heap-use-after-free', 'ThreadSanitizer: data race'")
    first_project_frame: str | None = None
    summary: str | None = None


class SmokeRunResult(BaseModel):
    preset: str
    ok: bool
    ran_for_s: float
    exit_code: int | None = Field(None, description="None if the server stopped it after the time limit (expected)")
    engines_skipped: list[str] = Field(default_factory=list, description="Compute engines reported unavailable")
    sanitizer_findings: list[SanitizerFinding] = Field(default_factory=list)
    output_tail: str
    log_resource: str


class TestInfo(BaseModel):
    name: str
    labels: list[str] = Field(default_factory=list)


class EnvironmentInfo(BaseModel):
    repo_root: str
    tools: dict[str, str | None] = Field(description="Tool -> version line, None if not installed")
    gpus: list[str]
    project_options_submodule: str
    notes: list[str] = Field(default_factory=list)
