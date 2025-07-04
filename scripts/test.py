import os
import sys
from typing import List

GREEN = "\033[92m"
RED   = "\033[91m"
RESET = "\033[0m"

TEMP_OUT = "/tmp/test_out"

failures = 0
passes = 0

def failure(test: str, reason: str):
    global failures
    failures += 1
    raise RuntimeError(f"{test} failed: {reason}")


def success(test: str):
    global passes
    passes += 1
    print(GREEN + f"{test} passed" + RESET)


def remove_extension(file: str) -> str:
    return file.rsplit(".", maxsplit=1)[0]


def compare(test, actual: List[int], expected: List[int]):
    for i, (a, e) in enumerate(zip(actual, expected)):
        bank = i // 0x20
        byte = i % 0x20
        if a != e:
            failure(test, f"mismatch on {bank:02x}:{byte:02x}: {a:02X} != {e:02X}")


def load(filename):
    with open(filename, "r") as file:
        return list(map(int, file.read().split()))


def run_test(test, tests_dir, executable):
    out_file = os.path.join(tests_dir, remove_extension(test) + ".out")
    
    if not os.path.exists(out_file):
        failure(test, f"could not find output file '{out_file}'")

    expected = load(out_file)
    
    test_file = os.path.join(tests_dir, test)
    res = os.system(f"{executable} {test_file} {TEMP_OUT} -r > /dev/null 2> /dev/null")
    
    if res != 0:
        failure(test, f"compiler exited with exit code {res}")
    
    actual = load(TEMP_OUT)
    
    compare(test, actual, expected)

    success(test)


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} [executable] [tests directory]")
        exit(1)
    
    executable = os.path.abspath(sys.argv[1])
    tests_dir = os.path.abspath(sys.argv[2])
    
    tests = [ file for file in os.listdir(tests_dir) if file.endswith(".acf") ]

    for test in tests:
        try:
            run_test(test, tests_dir, executable)
        except Exception as e:
            print(RED + str(e) + RESET)

    print(f"{passes} / {passes + failures} passed.")


if __name__ == "__main__":
    main()
