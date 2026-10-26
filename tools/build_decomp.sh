#!/usr/bin/env bash
# Builds the decomp's US version (lib/marioparty2's commit) from your ROM and brings its ELF back for
# tools/gen_syms.sh. It runs in tools/docker/Dockerfile.decomp's image with Docker, here or on a Linux host over ssh
# (BUILD_HOST). Never in CI: the build needs the ROM.
#
# `make setup` splits the ROM's code and data with splat (marioparty2.yaml), `make` builds them with KMC gcc 2.7.2 and
# checks the ROM it links against the one you gave it. gcc puts every function, static ones too, in the ELF's symbol
# table, so the ELF is used as it is linked. Into build/decomp/: marioparty2.elf. Nothing from it is committed but what
# tools/gen_syms.sh writes from it.
#
# Its requirements.txt pins a branch of splat without splat's N64 extras (n64img, pygfxd, crunch64), which are installed
# beside it, and its Makefile runs splat from the PATH, so the venv goes on it.
#
# Usage: [BUILD_HOST=<ssh host>] [DECOMP_DIR=<folder under ~, default marioparty2-decomp>] tools/build_decomp.sh [roms/marioparty2.us.z64]
set -euo pipefail
cd "$(dirname "$0")/.."
rom="${1:-roms/marioparty2.us.z64}"
sha1="166eda1c05670d337e2c3f15a5db528ae1e5d6e3"
[[ "$(sha1sum "$rom" | cut -d' ' -f1)" == "$sha1" ]] || { echo "$rom isn't Mario Party 2 (USA) as a .z64" >&2; exit 1; }
commit="$(git ls-tree HEAD lib/marioparty2 | awk '{print $3}')"
image="marioparty2-decomp:$(git hash-object tools/docker/Dockerfile.decomp | cut -c1-12)"
work="${DECOMP_DIR:-marioparty2-decomp}/${commit:0:12}"

if [[ -n "${BUILD_HOST:-}" ]]; then
    run() { ssh "$BUILD_HOST" "cd && $1"; }
    put() { scp -q "$1" "$BUILD_HOST:$2"; }
    get() { scp -q "$BUILD_HOST:$1" "$2"; }
else
    run() { (cd ~ && bash -c "$1"); }
    put() { cp "$1" ~/"$2"; }
    get() { cp ~/"$1" "$2"; }
fi

run "mkdir -p $work.docker"
put tools/docker/Dockerfile.decomp "$work.docker/Dockerfile"
run "docker image inspect $image > /dev/null 2>&1 || docker build -q -t $image $work.docker > /dev/null"
run "[ -d $work/.git ] || git clone -q https://github.com/mariopartyrd/marioparty2 $work
     git -C $work checkout -q $commit && git -C $work submodule update -q --init --recursive"
put "$rom" "$work/baserom.us.z64"
run "docker run --rm --name marioparty2-decomp-${commit:0:12} -u \$(id -u):\$(id -g) -e HOME=/tmp -v \$HOME/$work:/mp -w /mp $image bash -c '
    set -e
    export PATH=/mp/venv/bin:\$PATH
    [ -x venv/bin/python3 ] || python3 -m venv venv
    venv/bin/python3 -c \"import n64img\" 2> /dev/null || venv/bin/pip install -q -r requirements.txt n64img pygfxd crunch64
    make setup > setup.log 2>&1 || { tail -30 setup.log; exit 1; }
    make -j\$(nproc) > build.log 2>&1 || { tail -30 build.log; exit 1; }
    tail -1 build.log | grep -qx OK || { echo \"the build does not match the ROM\"; tail -5 build.log; exit 1; }'"

mkdir -p build/decomp
get "$work/build/marioparty2.elf" build/decomp/
echo "build/decomp/: the decomp's ELF at ${commit:0:12}, matching the ROM; next, tools/gen_syms.sh"
