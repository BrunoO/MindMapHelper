#!/usr/bin/env python3
"""
Filter clang-tidy false positives for cppcoreguidelines-init-variables in C++17 init-statements.
"""

import re
import sys
from pathlib import Path


def is_init_statement_warning(file_path: str, line_num: int, var_name: str) -> bool:
    try:
        path = Path(file_path)
        if not path.exists():
            return False

        with open(path, "r", encoding="utf-8", errors="ignore") as file:
            lines = file.readlines()

        if line_num < 1 or line_num > len(lines):
            return False

        line = lines[line_num - 1]

        patterns = [
            rf"\bif\s*\(\s*(?:const\s+)?(?:\w+\s+)*{re.escape(var_name)}\s*=\s*[^;]+;",
            rf"\bswitch\s*\(\s*(?:const\s+)?(?:\w+\s+)*{re.escape(var_name)}\s*=\s*[^;]+;",
            rf"\bfor\s*\(\s*(?:const\s+)?(?:\w+\s+)*{re.escape(var_name)}\s*=\s*[^;]+;",
            rf"\bwhile\s*\(\s*(?:const\s+)?(?:\w+\s+)*{re.escape(var_name)}\s*=\s*[^;]+;",
        ]
        for pattern in patterns:
            if re.search(pattern, line):
                return True

        if re.search(rf"\bauto\s*\[[^\]]*{re.escape(var_name)}", line):
            return True

        return False
    except Exception:
        return False


def filter_clang_tidy_output(input_lines):
    warning_pattern = re.compile(
        r'^([^:]+):(\d+):(\d+):\s+warning:\s+variable\s+[\'"](\w+)[\'"]\s+is\s+not\s+initialized\s+\[cppcoreguidelines-init-variables\]'
    )

    for line in input_lines:
        match = warning_pattern.match(line)
        if match:
            file_path = match.group(1)
            line_num = int(match.group(2))
            var_name = match.group(4)
            if is_init_statement_warning(file_path, line_num, var_name):
                continue
        yield line


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    try:
        for line in filter_clang_tidy_output(sys.stdin):
            print(line, end="")
    except KeyboardInterrupt:
        sys.exit(1)
    except BrokenPipeError:
        sys.exit(0)


if __name__ == "__main__":
    main()
