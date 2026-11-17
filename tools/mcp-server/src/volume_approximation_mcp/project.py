"""Repository layout and CMake presets: the only configurations the server will touch."""

from __future__ import annotations

import json
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

# Mirrors the default in cmake/Sanitizers.cmake (applied to Debug builds only)
DEFAULT_DEBUG_SANITIZERS = "address;undefined"

# Build targets are passed to `cmake --build --target`; keep them to plain names
_TARGET_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.+-]{0,127}$")


class ProjectError(ValueError):
    """Invalid input from the client, reported back as a tool error."""


def find_repo_root() -> Path:
    """VOLUME_APPROXIMATION_ROOT if set, else the nearest parent with CMakePresets.json."""
    if env := os.environ.get("VOLUME_APPROXIMATION_ROOT"):
        return Path(env).resolve()
    for parent in Path(__file__).resolve().parents:
        if (parent / "CMakePresets.json").is_file() and (parent / "ApproximationLib").is_dir():
            return parent
    raise RuntimeError("VolumeApproximation repository not found; set VOLUME_APPROXIMATION_ROOT")


@dataclass(frozen=True)
class Preset:
    name: str
    display_name: str
    build_type: str
    cuda_enabled: bool
    sanitizers: tuple[str, ...]
    binary_dir: Path

    @property
    def state_dir(self) -> Path:
        """Where the server keeps logs and the last results for this preset."""
        return self.binary_dir / ".mcp"


def _cache_value(value: Any) -> str:
    # cacheVariables entries are either "VALUE" or {"type": ..., "value": ...}
    if isinstance(value, dict):
        return str(value.get("value", ""))
    if isinstance(value, bool):
        return "ON" if value else "OFF"
    return str(value)


def _is_on(value: str) -> bool:
    return value.strip().upper() in {"ON", "TRUE", "1", "YES", "Y"}


def load_presets(root: Path) -> dict[str, Preset]:
    """Non-hidden configure presets from CMakePresets.json, with `inherits` resolved."""
    data = json.loads((root / "CMakePresets.json").read_text(encoding="utf-8"))
    raw: dict[str, dict[str, Any]] = {p["name"]: p for p in data.get("configurePresets", [])}

    def resolve(name: str, chain: tuple[str, ...] = ()) -> dict[str, Any]:
        if name in chain:
            raise ValueError(f"Preset inheritance cycle: {' -> '.join((*chain, name))}")
        if name not in raw:
            raise ValueError(f"Preset {chain[-1]!r} inherits unknown preset {name!r}")
        preset = raw[name]
        parents = preset.get("inherits", [])
        if isinstance(parents, str):
            parents = [parents]
        merged: dict[str, Any] = {"cacheVariables": {}}
        # CMake: earlier parents win over later ones, the preset's own fields win over all
        for parent in reversed(parents):
            resolved = resolve(parent, (*chain, name))
            merged.update({k: v for k, v in resolved.items() if k != "cacheVariables"})
            merged["cacheVariables"].update(resolved["cacheVariables"])
        merged.update({k: v for k, v in preset.items() if k not in {"cacheVariables", "inherits", "hidden"}})
        merged["cacheVariables"].update(preset.get("cacheVariables", {}))
        return merged

    presets: dict[str, Preset] = {}
    for name, preset in raw.items():
        if preset.get("hidden"):
            continue
        resolved = resolve(name)
        cache = {k: _cache_value(v) for k, v in resolved["cacheVariables"].items()}
        binary_dir = str(resolved.get("binaryDir", "")).replace("${sourceDir}", str(root))
        binary_dir = binary_dir.replace("${presetName}", name)
        if not binary_dir or "${" in binary_dir:
            raise ValueError(f"Preset {name!r}: unsupported binaryDir {resolved.get('binaryDir')!r}")
        build_type = cache.get("CMAKE_BUILD_TYPE", "")
        sanitizers: tuple[str, ...] = ()
        if build_type == "Debug":
            value = cache.get("DEBUG_SANITIZERS", DEFAULT_DEBUG_SANITIZERS)
            sanitizers = tuple(s for s in value.split(";") if s)
        presets[name] = Preset(
            name=name,
            display_name=str(resolved.get("displayName", name)),
            build_type=build_type,
            cuda_enabled=_is_on(cache.get("CUDA_ENABLED", "OFF")),
            sanitizers=sanitizers,
            binary_dir=Path(binary_dir),
        )
    return presets


def require_preset(presets: dict[str, Preset], name: str) -> Preset:
    """Only presets from CMakePresets.json are accepted: never arbitrary CMake arguments."""
    if name not in presets:
        raise ProjectError(f"Unknown preset {name!r}. Available: {', '.join(sorted(presets))}")
    return presets[name]


def require_target(target: str) -> str:
    if not _TARGET_RE.fullmatch(target):
        raise ProjectError(f"Invalid build target {target!r}: use a plain CMake target name")
    return target


def require_test_regex(regex: str) -> str:
    """Passed to `ctest -R` as a separate argument (no shell), so only bound its size."""
    if not regex or len(regex) > 200 or "\n" in regex or "\x00" in regex:
        raise ProjectError("Test filter must be a single-line regular expression of at most 200 characters")
    try:
        re.compile(regex)
    except re.error as e:
        raise ProjectError(f"Invalid test filter regex: {e}") from e
    return regex
