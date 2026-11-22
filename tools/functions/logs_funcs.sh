#!/bin/bash
info() { printf '[info] %s\n' "$*"; }
warning() { printf '[warning] %s\n' "$*" >&2; }
error() { printf '[error] %s\n' "$*" >&2; return 1; }
install_status() { installer_set DI_INSTALLER_STATUS "$1"; }
print_system_info() { cat /etc/os-release; uname -m; }
