Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

cmake -S . -B build -A x64 -DBM_ENABLE_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
