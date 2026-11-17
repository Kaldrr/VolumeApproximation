import json

import pytest

from volume_approximation_mcp import project


def test_real_presets_resolve(tmp_path):
    presets = project.load_presets(project.find_repo_root())
    assert {"cpu-debug", "cpu-release", "cuda-debug", "cuda-tsan", "cuda-relwithdebinfo"} <= presets.keys()
    assert "base" not in presets and "cuda" not in presets, "hidden presets are not configurations"
    cpu = presets["cpu-debug"]
    assert (cpu.build_type, cpu.cuda_enabled, cpu.sanitizers) == ("Debug", False, ("address", "undefined"))
    assert presets["cuda-tsan"].sanitizers == ("thread", "undefined")
    assert presets["cuda-debug"].cuda_enabled
    assert presets["cpu-release"].sanitizers == ()
    assert cpu.binary_dir.parts[-2:] == ("build", "cpu-debug")


def test_inheritance_precedence(tmp_path):
    (tmp_path / "CMakePresets.json").write_text(json.dumps({
        "version": 6,
        "configurePresets": [
            {"name": "a", "hidden": True, "binaryDir": "${sourceDir}/out/${presetName}",
             "cacheVariables": {"CUDA_ENABLED": "OFF", "CMAKE_BUILD_TYPE": "Release"}},
            {"name": "b", "hidden": True, "cacheVariables": {"CUDA_ENABLED": "ON"}},
            # earlier parent wins over later; own values win over parents
            {"name": "c", "inherits": ["b", "a"], "cacheVariables": {"CMAKE_BUILD_TYPE": {"type": "STRING", "value": "Debug"}}},
        ],
    }))
    c = project.load_presets(tmp_path)["c"]
    assert c.cuda_enabled and c.build_type == "Debug"
    assert c.binary_dir == tmp_path / "out" / "c"


@pytest.mark.parametrize("name", ["cpu-debug; rm -rf ~", "../etc", "", "--fresh", "CPU-DEBUG"])
def test_unknown_presets_rejected(name):
    presets = project.load_presets(project.find_repo_root())
    with pytest.raises(project.ProjectError, match="Unknown preset"):
        project.require_preset(presets, name)


@pytest.mark.parametrize("target", ["ApproximationLib", "VolumeApproximation", "ApproximationLibTests", "all"])
def test_valid_targets(target):
    assert project.require_target(target) == target


@pytest.mark.parametrize("target", ["--fresh", "-j1000", "a b", "a;b", "$(id)", "x" * 200, ""])
def test_invalid_targets(target):
    with pytest.raises(project.ProjectError):
        project.require_target(target)


@pytest.mark.parametrize("regex", ["", "a\nb", "(unclosed", "x" * 201])
def test_invalid_test_regex(regex):
    with pytest.raises(project.ProjectError):
        project.require_test_regex(regex)
