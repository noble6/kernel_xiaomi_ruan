#!/bin/bash
# Build the experimental 6.6 kernel for ruan: ruan/build66.sh [make targets]
#
# Needs AOSP clang r510928 (the android15-6.6 compiler) in $CLANG, default
# ../clang-r510928 next to this tree. Output goes to $O, default ../out-66.
set -e
K=$(cd "$(dirname "$0")/.." && pwd)
CLANG=${CLANG:-$K/../clang-r510928}
O=${O:-$K/../out-66}
export PATH=$CLANG/bin:$PATH
M="make -C $K O=$O ARCH=arm64 LLVM=1 LLVM_IAS=1 -j$(nproc)"
$M gki_defconfig
"$K"/scripts/kconfig/merge_config.sh -m -O "$O" "$O"/.config \
	"$K"/arch/arm64/configs/vendor/parrot_perf.config \
	"$K"/arch/arm64/configs/vendor/ruan.config
$M olddefconfig
$M ${*:-Image modules}
