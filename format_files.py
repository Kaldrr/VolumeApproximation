from multiprocessing import Pool
from pathlib import Path
from subprocess import run


def format_file(file: Path):
    run(['clang-format', '-style=file', '-i', file], check=True)


if __name__ == '__main__':
    current_dir = Path.cwd()
    with Pool() as pool:
        tasks = []

        for file in current_dir.rglob('*.cpp'):
            tasks.append(pool.apply_async(format_file, (file,)))
        for file in current_dir.rglob('*.h'):
            tasks.append(pool.apply_async(format_file, (file,)))
        for task in tasks:
            task.wait()
        print('Done!')
