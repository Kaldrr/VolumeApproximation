# volume-approximation MCP server

Exposes VolumeApproximation's CMake presets, builds, CTest results and headless app runs
to Claude Code (or any MCP client) over stdio. It's written with the official Python SDK
(`mcp` 2.x, `MCPServer`) and managed with `uv`.

It's registered for the project in the repo's `.mcp.json`, so Claude Code offers it in this
repository after you approve it. To run it by hand:

```bash
uv run --directory tools/mcp-server volume-approximation-mcp   # speaks MCP on stdin/stdout
```

## What it offers

| Tool | Does | Returns |
|---|---|---|
| `list_configurations` | Reads `CMakePresets.json` | Build type, CUDA on/off, sanitizers, build dir, configured?, last result, usable here? |
| `configure(preset, fresh)` | `cmake --preset` (`--fresh` drops the cache) | Like `build` |
| `build(preset, target?)` | Configures if needed, then `cmake --build --preset` | Parsed errors and project warnings (file, line, column, source context), counts, duration. Reports progress. |
| `get_diagnostics(preset, severity, file)` | Filters the last build's diagnostics | List of diagnostics |
| `list_tests(preset)` | `ctest --show-only=json-v1` | Test names |
| `run_tests(preset, name_regex?, build_first)` | Builds, then `ctest --preset --output-junit` | Pass/fail/skip counts; failures with gtest assertion and sanitizer lines |
| `smoke_run(preset, seconds)` | Runs the app with `QT_QPA_PLATFORM=offscreen` | Sanitizer findings, engines reported unavailable |
| `environment` | Tool versions, `nvidia-smi -L`, submodule status | Facts plus hints (e.g. "no GPU: CUDA tests will skip") |

There are also three resources with the full logs, `build://{preset}/build-log`, `test-log` and
`smoke-log` (tailed beyond 200 KB), and two prompts, `fix_build` and `verify_change`.

Logs and the last build state live in `build/<preset>/.mcp/`.

## Design and security choices

- **Closed set of operations.** Presets must exist in `CMakePresets.json`. Targets must be plain
  names (`^[A-Za-z0-9_][A-Za-z0-9_.+-]*$`, so no `--fresh` or `-j1000`). Test filters are bounded
  regexes. Nothing accepts free-form commands or CMake arguments, so a prompt injection can at
  worst build or test an existing preset.
- **No shell.** Subprocesses are started with argument lists (`create_subprocess_exec`).
- **stdin is `/dev/null` for every child.** With stdio transport the server's stdin is the
  protocol channel. An inherited stdin would let `cmake`, a compiler or the app read protocol
  bytes. For the same reason the server never prints to stdout. Logging goes to stderr.
- **Whole process trees are stopped.** Children run in their own process group, so a timeout
  or a cancelled tool call stops cmake → ninja → compilers, not just the top process.
- **One job at a time.** Builds, test runs and smoke runs share one lock, because each already
  uses every core and two builds in one build dir would corrupt it.
- **Small results.** Tools return parsed findings (at most 40 diagnostics, 20 failures, with
  excerpts capped). Full logs are resources, read only when needed.
- **Hints, not guarantees.** Tool annotations (`read_only_hint` etc.) tell the client what's safe
  to auto-approve. `build`/`run_tests` are non-destructive and idempotent, but not read-only
  (they write to `build/`).

Builds are slow on a cold cache (minutes for CUDA). The server allows 45 minutes per build and
20 per test run. If your client gives up earlier, raise its tool timeout. Claude Code reads
`MCP_TOOL_TIMEOUT` (milliseconds).

## Development

```bash
cd tools/mcp-server
uv run pytest                      # parsers, preset/input validation, in-process MCP client tests
uv run python scripts/e2e.py       # real stdio session: builds + tests cpu-debug, calls every tool
uv run python scripts/e2e.py cuda-debug
```

For interactive poking, the MCP Inspector works too:
`npx @modelcontextprotocol/inspector uv run --directory tools/mcp-server volume-approximation-mcp`.
