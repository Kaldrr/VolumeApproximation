"""Drive the server through a real MCP client session, in-process (no subprocess, no builds)."""

import pytest
from mcp import Client

from volume_approximation_mcp.server import mcp


# Each test opens its own session: an async yield-fixture would close the session in a
# different task than it was opened in, which anyio's cancel scopes reject.


async def test_tools_are_listed_with_annotations():
    async with Client(mcp) as client:
        tools = {t.name: t for t in (await client.list_tools()).tools}
        assert set(tools) == {"list_configurations", "configure", "build", "get_diagnostics", "list_tests",
                              "run_tests", "smoke_run", "environment"}
        assert tools["list_configurations"].annotations.read_only_hint
        assert not tools["build"].annotations.read_only_hint
        assert tools["build"].annotations.destructive_hint is False
        assert "preset" in tools["build"].input_schema["properties"]
        assert "ctx" not in tools["build"].input_schema["properties"], "Context must not leak into the schema"


async def test_list_configurations_structured():
    async with Client(mcp) as client:
        result = await client.call_tool("list_configurations", {})
        assert not result.is_error
        presets = {p["name"]: p for p in result.structured_content["result"]}
        assert presets["cpu-debug"]["cuda_enabled"] is False
        assert presets["cpu-debug"]["build_dir"] == "build/cpu-debug"


@pytest.mark.parametrize("tool,args", [
    ("build", {"preset": "cpu-debug; touch /tmp/pwned"}),
    ("build", {"preset": "cpu-debug", "target": "--fresh"}),
    ("run_tests", {"preset": "cpu-debug", "name_regex": "(broken"}),
    ("get_diagnostics", {"preset": "../../etc"}),
    ("smoke_run", {"preset": "nope"}),
])
async def test_bad_input_is_a_tool_error(tool, args):
    async with Client(mcp) as client:
        result = await client.call_tool(tool, args)
        assert result.is_error


async def test_prompts_and_resource_templates():
    async with Client(mcp) as client:
        prompts = {p.name for p in (await client.list_prompts()).prompts}
        assert {"fix_build", "verify_change"} <= prompts
        templates = {t.uri_template for t in (await client.list_resource_templates()).resource_templates}
        assert "build://{preset}/build-log" in templates
