#!/usr/bin/env bash
# PostToolUse (Edit|Write): run clang-format on the C++/CUDA file Claude just changed.
# Input: hook JSON on stdin. Never blocks: formatting problems are not worth stopping for.

file=$(jq -r '.tool_input.file_path // .tool_response.filePath // empty')
[[ -z "$file" || ! -f "$file" ]] && exit 0

case "$file" in
  "$CLAUDE_PROJECT_DIR"/project_options/* | "$CLAUDE_PROJECT_DIR"/build*/*) exit 0 ;; # submodule / build output
  *.cpp | *.hpp | *.h | *.cu | *.cuh) ;;
  *) exit 0 ;;
esac

clang-format -i "$file" 2>/dev/null || true
