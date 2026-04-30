# Pre-commit Hook for clang-tidy

This folder contains a pre-commit hook script that runs `clang-tidy` on staged C++ files (`.cpp`, `.h`, `.hpp`, `.cxx`, `.cc`) and blocks the commit when warnings/errors are detected.

## Install

### Option 1: copy (recommended)
```bash
cp scripts/pre-commit-clang-tidy.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### Option 2: symlink
```bash
ln -s ../../scripts/pre-commit-clang-tidy.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Requirements

- `clang-tidy` available in PATH (or Homebrew LLVM default paths)
- `.clang-tidy` present at repo root
- `compile_commands.json` present at repo root, or in `build/`

Generate compile commands if missing:
```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## Bypass (not recommended)

```bash
git commit --no-verify
```

## Manual test

```bash
git add src/some_file.cpp
.git/hooks/pre-commit
```
