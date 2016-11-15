inherit aspeed_uboot_image

# /dev
require recipes-core/images/aspeed-dev.inc

# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"
UBOOT_IMAGE_ENTRYPOINT = "0x40800000"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python \
  packagegroup-openbmc-rest \
  at93cx6-util \
  bcm5396-util \
  bitbang \
  rackmon \
  cpldupdate \
  flashrom \
  ipmid \
  lldp-util \
  memtester  \
  mkeeprom  \
  openbmc-utils \
  openbmc-gpio \
  po-eeprom \
  repeater  \
  retimer  \
  sensor-setup \
  sms-kcsd \
  stress  \
  usb-console \
  watchdog-ctrl \
  wedge-eeprom \
  spatula \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "
