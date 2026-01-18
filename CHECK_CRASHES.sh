#!/bin/bash

echo "=== Checking for Recent Crash Logs ==="
echo ""

# Check for crashes in the last hour
find ~/Library/Logs/DiagnosticReports -name "*ModalAttractors*" -mtime -1h 2>/dev/null | while read crash; do
    echo "Found crash: $crash"
    echo "---"
    head -50 "$crash"
    echo ""
done

# Check for extension process crashes
find ~/Library/Logs/DiagnosticReports -name "*Extension*" -mtime -1h 2>/dev/null | grep -i modal | while read crash; do
    echo "Found extension crash: $crash"
    echo "---"
    head -50 "$crash"
    echo ""
done

echo ""
echo "=== Checking System Log for Plugin ==="
echo ""
log show --predicate 'subsystem == "com.bund.media.ModalAttractorsExtension"' --last 5m --info 2>/dev/null | head -100

echo ""
echo "=== Done ==="
