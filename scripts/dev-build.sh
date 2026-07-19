#!/usr/bin/env bash
set -euo pipefail

if [ ! -f configure.ac ]; then
  echo "configure.ac not found; did you run from repo root?" >&2
  exit 1
fi

autoreconf --install
./configure --host=i686-gnu CC='gcc -m32' LD='ld -melf_i386' MIG='mig'
make -j"$(nproc)"
make check
