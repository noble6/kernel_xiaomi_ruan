# SPDX-License-Identifier: GPL-2.0-only
#
# Qualcomm's securemsm-kernel (CodeLinaro LA.VENDOR.15.4.7.r1-05600-pakala,
# 3031fa388b38), built into the kernel: the parts ruan's vendor loaded as
# modules on 5.10 (qseecom, smcinvoke, tz_log, qce50/qcedev, hdcp).
# smcinvoke is the si_core flavour, which 6.6 has built in.

ccflags-y += -I$(srctree)/$(src) \
	     -I$(srctree)/$(src)/linux \
	     -I$(srctree)/$(src)/include/linux \
	     -I$(srctree)/$(src)/include/uapi \
	     -I$(srctree)/$(src)/include/uapi/linux \
	     -include $(srctree)/$(src)/config/sec-kernel_defconfig.h \
	     -include $(srctree)/$(src)/config/sec-kernel_defconfig_qseecom_compat.h \
	     -include $(srctree)/$(src)/config/sec-kernel_defconfig_smcinvoke.h

obj-y += qseecom_dlkm.o
qseecom_dlkm-y := qseecom/qseecom.o
qseecom_dlkm-$(CONFIG_COMPAT) += qseecom/qseecom_32bit_impl.o

obj-y += smcinvoke_dlkm.o
smcinvoke_dlkm-y := smcinvoke/si_core_xts/qseecom.o \
		    smcinvoke/si_core_xts/smci_kernel.o \
		    smcinvoke/si_core_xts/smci.o

obj-y += tz_log_dlkm.o
tz_log_dlkm-y := tz_log/tz_log.o

obj-y += qce50_dlkm.o
qce50_dlkm-y := crypto-qti/qce50.o

obj-y += qcedev-mod_dlkm.o
qcedev-mod_dlkm-y := crypto-qti/qcedev.o crypto-qti/qcedev_smmu.o
qcedev-mod_dlkm-$(CONFIG_COMPAT) += crypto-qti/compat_qcedev.o

obj-y += hdcp_qseecom_dlkm.o
hdcp_qseecom_dlkm-y := hdcp/hdcp_main.o hdcp/hdcp_smcinvoke.o hdcp/hdcp_qseecom.o
