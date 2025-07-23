#!/bin/bash
echo "Starting LibreOffice debug test..."
export SAL_LOG="+WARN+INFO"
./instdir/LibreOfficeDev.app/Contents/MacOS/soffice --writer --nologo 2>&1 | tee /tmp/lo_debug.log
echo "Debug output saved to /tmp/lo_debug.log"

