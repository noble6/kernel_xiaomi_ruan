# SPDX-License-Identifier: GPL-2.0-only
#
# Qualcomm's display stack for ruan, built into the kernel:
#  display/     MiCode display-drivers annibale-w-oss (4a586833e811)
#  mm-drivers/  CodeLinaro mm-drivers display-kernel.lnx.12.13.r1-rel
#               (msm_ext_display and sync_fence; parrot has no hw fences)
#  graphics/    CodeLinaro graphics-kernel gfx-kernel.lnx.15.0.r12-rel (kgsl)
export DISPLAY_ROOT := $(srctree)/techpack/display
export MSM_EXT_DISPLAY_ROOT := $(srctree)/techpack/mm-drivers/
export SYNC_FENCE_ROOT := $(srctree)/techpack/mm-drivers/
export MSM_HW_FENCE_ROOT := $(srctree)/techpack/mm-drivers/
export KERNEL_SRC := $(srctree)
export KERNEL_ROOT := $(srctree)
export KGSL_PATH := $(srctree)/techpack/graphics

obj-y += mm-drivers/msm_ext_display/
obj-y += mm-drivers/sync_fence/
obj-y += display/
obj-y += graphics/
