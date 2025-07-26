#!/bin/bash
echo "Starting LibreOffice with debug logging for SmartRewrite..."
SAL_LOG="+WARN+INFO.sw.smartrewrite" DYLD_LIBRARY_PATH=instdir/LibreOfficeDev.app/Contents/Frameworks ./instdir/LibreOfficeDev.app/Contents/MacOS/soffice --nologo --nodefault --writer 2>&1 | tee /tmp/libreoffice_smartrewrite_debug.log
