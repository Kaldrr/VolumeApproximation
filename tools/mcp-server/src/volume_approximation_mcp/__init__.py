"""MCP server for the VolumeApproximation project."""


def main() -> None:
    from .server import main as run

    run()
