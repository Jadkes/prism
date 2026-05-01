#!/bin/bash
# install_deps.sh - Install required packages for c_tester
# Purpose: Install GCC sanitizer libraries (libasan, libubsan) and valgrind
sudo dnf install -y gcc libasan libubsan valgrind
