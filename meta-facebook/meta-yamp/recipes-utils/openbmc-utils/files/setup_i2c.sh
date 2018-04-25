#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

# Bus  0 - SMBus 1
i2c_device_add 0 0x20 bmctpm

# Bus  1 - SMBus 2
i2c_device_add 1 0x50 supsfp

# Bus  2 - SMBus 3 - LC, MUXED as Bus 16-23
i2c_device_add 2 0x75 pca9548

# Bus  4 - SMBus 5 SCD
i2c_device_add 4 0x23 scdfpga
i2c_device_add 4 0x4d max6581
i2c_device_add 4 0x50 24c512

# Bus  5 - SMBus 6 PSU1
i2c_device_add 5 0x50 psu1

# Bus  6 - SMBus 7 PSU2
i2c_device_add 6 0x50 psu2

# Bus  7 - SMBus 8 PSU3
i2c_device_add 7 0x50 psu3

# Bus  8 - SMBus 9 PSU4
i2c_device_add 8 0x50 psu4

# Bus  9 - SMBus 10 SUP DPM
i2c_device_add 9 0x4e ucd90320

# Bus  10 - SMBus 11 SUP POWER
i2c_device_add 10 0x21 cpupwr
i2c_device_add 10 0x27 mempwr

# Bus  11 - SMBus 12 SUP TEMP
i2c_device_add 11 0x40 supecb
i2c_device_add 11 0x4c max6658

# Bus  12 - SMBus 13 SUP CPLD
i2c_device_add 12 0x43 supcpld

# Bus  13 - SMBus 14 FAN CTRL
i2c_device_add 13 0x4c max6658
i2c_device_add 13 0x50 24c512
i2c_device_add 13 0x60 fanctrl

# Bus  16 - Muxed LC1 
i2c_device_add 14 0x4e ucd90120
i2c_device_add 14 0x42 lcecb
i2c_device_add 14 0x20 pca9555
i2c_device_add 14 0x4c max6658
i2c_device_add 14 0x50 24c512

# Bus  17 - Muxed LC2 
i2c_device_add 15 0x4e ucd90120
i2c_device_add 15 0x42 lcecb
i2c_device_add 15 0x20 pca9555
i2c_device_add 15 0x4c max6658
i2c_device_add 15 0x50 24c512

# Bus  18 - Muxed LC3 
i2c_device_add 16 0x4e ucd90120
i2c_device_add 16 0x42 lcecb
i2c_device_add 16 0x20 pca9555
i2c_device_add 16 0x4c max6658
i2c_device_add 16 0x50 24c512

# Bus  19 - Muxed LC4 
i2c_device_add 17 0x4e ucd90120
i2c_device_add 17 0x42 lcecb
i2c_device_add 17 0x20 pca9555
i2c_device_add 17 0x4c max6658
i2c_device_add 17 0x50 24c512

# Bus  20 - Muxed LC5 
i2c_device_add 18 0x4e ucd90120
i2c_device_add 18 0x42 lcecb
i2c_device_add 18 0x20 pca9555
i2c_device_add 18 0x4c max6658
i2c_device_add 18 0x50 24c512

# Bus  21 - Muxed LC6 
i2c_device_add 19 0x4e ucd90120
i2c_device_add 19 0x42 lcecb
i2c_device_add 19 0x20 pca9555
i2c_device_add 19 0x4c max6658
i2c_device_add 19 0x50 24c512

# Bus  22 - Muxed LC7 
i2c_device_add 20 0x4e ucd90120
i2c_device_add 20 0x42 lcecb
i2c_device_add 20 0x20 pca9555
i2c_device_add 20 0x4c max6658
i2c_device_add 20 0x50 24c512

# Bus  23 - Muxed LC8 
i2c_device_add 21 0x4e ucd90120
i2c_device_add 21 0x42 lcecb
i2c_device_add 21 0x20 pca9555
i2c_device_add 21 0x4c max6658
i2c_device_add 21 0x50 24c512

# Bus  3 - SMBus 4 SCB
i2c_device_add 3 0x40 scbecb1
i2c_device_add 3 0x41 scbecb2
i2c_device_add 3 0x4e ucd90120
i2c_device_add 3 0x12 scbvrd1
i2c_device_add 3 0x14 scbvrd2
