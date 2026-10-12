#!/bin/sh
# Builds and runs the host-side meta.txt parser tests.
#
# These need no Flipper SDK and no hardware: anim_meta.c is plain C with no
# SDK includes precisely so this is possible.  Sanitizers are on because the
# parser walks a non-NUL-terminated buffer and writes into fixed arrays.
set -e
cd "$(dirname "$0")"
mkdir -p build
gcc -std=gnu11 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined \
    -o build/test_anim_meta test_anim_meta.c ../../anim_meta.c
./build/test_anim_meta
