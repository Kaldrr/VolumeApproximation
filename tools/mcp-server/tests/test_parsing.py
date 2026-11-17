from pathlib import Path

from volume_approximation_mcp import parsing

ROOT = Path("/repo")

GCC_OUTPUT = """\
[3/14] Building CXX object ApproximationLib/CMakeFiles/ApproximationLib.dir/src/Foo.cpp.o
FAILED: [code=1] ApproximationLib/CMakeFiles/ApproximationLib.dir/src/Foo.cpp.o
/usr/bin/c++ -DFMT_SHARED -I/repo/ApproximationLib/include -g -std=c++23 -c /repo/ApproximationLib/src/Foo.cpp
\x1b[01m\x1b[K/repo/ApproximationLib/src/Foo.cpp:12:5:\x1b[m\x1b[K \x1b[01;31m\x1b[Kerror: \x1b[m\x1b[K'bar' was not declared in this scope
   12 |     bar();
      |     ^~~
/repo/ApproximationLib/src/Foo.cpp:3:7: note: candidate: 'baz'
/repo/ApproximationLib/src/Foo.cpp:20:9: warning: unused variable 'x' [-Wunused-variable]
/repo/build/cpu-debug/UserInterface/qrc_gen.cpp:5:1: warning: generated noise [-Wpedantic]
/usr/include/c++/16/bits/stl_algo.h:100:3: warning: system header noise
/repo/ApproximationLib/src/Foo.cpp:12:5: error: 'bar' was not declared in this scope
ninja: build stopped: subcommand failed.
"""


def test_gcc_errors_warnings_context_and_notes():
    diags = parsing.parse_diagnostics(GCC_OUTPUT, ROOT)
    errors = [d for d in diags if d.severity == "error"]
    assert len(errors) == 1, "duplicates must be removed"
    err = errors[0]
    assert (err.file, err.line, err.column) == ("ApproximationLib/src/Foo.cpp", 12, 5)
    assert "'bar' was not declared" in err.message
    assert err.context == ["   12 |     bar();", "      |     ^~~"]
    assert err.notes and "candidate" in err.notes[0]

    warnings = {d.file: d for d in diags if d.severity == "warning"}
    assert warnings["ApproximationLib/src/Foo.cpp"].in_project
    assert not warnings["build/cpu-debug/UserInterface/qrc_gen.cpp"].in_project
    assert not warnings["/usr/include/c++/16/bits/stl_algo.h"].in_project


def test_nvcc_cmake_and_linker_formats():
    output = """\
/repo/ApproximationLib/src/Cuda.cu(42): error: identifier "foo" is undefined
/repo/ApproximationLib/src/Cuda.cu(50): warning #177-D: variable "y" was declared but never referenced
CMake Error at ApproximationLib/CMakeLists.txt:7 (find_package):
  Could not find a package configuration file provided by "TBB".

/usr/bin/ld: CMakeFiles/x.dir/a.cpp.o: in function `main':
/usr/bin/ld: a.cpp:(.text+0x5): undefined reference to `Missing()'
collect2: error: ld returned 1 exit status
"""
    diags = parsing.parse_diagnostics(output, ROOT)
    by_source = {d.source: d for d in diags if d.severity == "error"}
    assert by_source["nvcc"].line == 42 and by_source["nvcc"].file == "ApproximationLib/src/Cuda.cu"
    assert any(d.source == "nvcc" and d.severity == "warning" and d.line == 50 for d in diags)
    assert by_source["cmake"].file == "ApproximationLib/CMakeLists.txt"
    assert "Could not find a package configuration file" in by_source["cmake"].message
    assert "undefined reference to `Missing()'" in by_source["linker"].message


def test_ninja_progress():
    assert parsing.ninja_progress("[3/14] Building CXX object a.o") == (3, 14, "Building CXX object a.o")
    assert parsing.ninja_progress("FAILED: a.o") is None


JUNIT = """<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="Linux" tests="4" failures="1" disabled="0" skipped="1" hostname="" time="1" timestamp="">
  <testcase name="Vector3.DotProduct" classname="Vector3.DotProduct" time="0.07" status="run">
    <system-out>[       OK ] Vector3.DotProduct</system-out>
  </testcase>
  <testcase name="CpuBackend.CubeVolume" classname="CpuBackend.CubeVolume" time="0.6" status="fail">
    <failure message="Failed"/>
    <system-out>/repo/tests/CpuBackendTests.cpp:40: Failure
The difference between result.Volume and expected is 9e+06, which exceeds 240000
[  FAILED  ] CpuBackend.CubeVolume (600 ms)</system-out>
  </testcase>
  <testcase name="CudaBackend.CubeVolume" classname="CudaBackend.CubeVolume" time="0.01" status="run">
    <skipped message="SKIP_REGULAR_EXPRESSION_MATCHED"/>
  </testcase>
  <testcase name="Missing.Exe" classname="Missing.Exe" time="0" status="notrun">
    <system-out>Could not find executable</system-out>
  </testcase>
</testsuite>
"""


def test_junit_statuses_and_excerpt():
    cases = {c.name: c for c in parsing.parse_junit(JUNIT)}
    assert cases["Vector3.DotProduct"].status == "passed"
    assert cases["CudaBackend.CubeVolume"].status == "skipped"
    assert cases["Missing.Exe"].status == "failed"
    failed = cases["CpuBackend.CubeVolume"]
    assert failed.status == "failed" and failed.duration_s == 0.6
    assert "CpuBackendTests.cpp:40: Failure" in failed.output_excerpt
    assert "exceeds 240000" in failed.output_excerpt


def test_sanitizer_reports_with_project_frames():
    output = """\
=================================================================
==123==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010 at pc 0x1 bp 0x2 sp 0x3
READ of size 4 at 0x602000000010 thread T0
    #0 0x55 in std::vector<int>::operator[] /usr/include/c++/16/bits/stl_vector.h:1100
    #1 0x56 in Helpers::Parse(Foo const&) /repo/UserInterface/GeometryHelpers.cpp:88
SUMMARY: AddressSanitizer: heap-use-after-free /repo/UserInterface/GeometryHelpers.cpp:88 in Helpers::Parse
/repo/ApproximationLib/include/ApproximationLib/Vector3.hpp:120:10: runtime error: signed integer overflow
WARNING: ThreadSanitizer: data race (pid=777)
  Write of size 8 at 0x7b0 by thread T2:
    #0 0x1 in ComputeRunnable::run() /repo/UserInterface/SceneController.cpp:130 (VolumeApproximation+0x1)
"""
    findings = parsing.parse_sanitizer_reports(output, ROOT)
    assert [f.kind for f in findings] == [
        "AddressSanitizer: heap-use-after-free",
        "UndefinedBehaviorSanitizer",
        "ThreadSanitizer: data race",
    ]
    assert findings[0].first_project_frame == "Helpers::Parse(Foo const&) at UserInterface/GeometryHelpers.cpp:88"
    assert findings[0].summary.startswith("SUMMARY: AddressSanitizer")
    assert findings[1].first_project_frame == "ApproximationLib/include/ApproximationLib/Vector3.hpp:120:10"
    assert findings[2].first_project_frame.endswith("UserInterface/SceneController.cpp:130")
