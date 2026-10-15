#!/bin/bash
# Multi-Process Simulator Launcher (Week 2)
# Starts Logger -> Core -> UI in strict dependency order

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building Simulator Binaries ==="
make all

cleanup() {
    echo ""
    echo "=== Shutting Down Processes ==="
    kill $CORE_PID 2>/dev/null || true
    kill $LOGGER_PID 2>/dev/null || true
    wait 2>/dev/null || true
    echo "=== All processes stopped ==="
}
trap cleanup EXIT INT TERM

echo ""
echo "=== Step 1: Starting Logger Process ==="
./logger &
LOGGER_PID=$!
sleep 1

echo "=== Step 2: Starting Core Process ==="
./core &
CORE_PID=$!
sleep 1

echo "=== Step 3: Starting UI Process (Interactive) ==="
./ui

echo "UI closed successfully."
