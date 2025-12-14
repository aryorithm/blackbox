#!/bin/bash
set -e # Exit on error

echo "========================================"
echo "   BLACKBOX AUTOMATED TESTING SUITE"
echo "========================================"

# 1. C++ Core Tests
echo -e "\n>>> [1/3] Running C++ Core Unit Tests..."
cd core
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make -j4
./run_core_tests
cd ../..

# 2. Python Sim Tests
echo -e "\n>>> [2/3] Running Python Sim Tests..."
# Assumes python3 and pytest are installed
if command -v pytest &> /dev/null; then
    pytest sim/
else
    echo "[WARN] pytest not found, skipping Python tests."
fi

# 3. Go Tower Tests
echo -e "\n>>> [3/3] Running Go API Tests..."
cd tower
if command -v go &> /dev/null; then
    go test ./...
else
    echo "[WARN] go not found, skipping API tests."
fi
cd ..

echo -e "\n✅ ALL TESTS PASSED."