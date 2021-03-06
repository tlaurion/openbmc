/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40

#define ADM1278_ADDR 0x10
#define LM75_1_ADDR 0x48
#define LM75_2_ADDR 0x4b

#define INTERVAL_MAX  5

static int
pim_driver_add(uint8_t num) {
  int ret = 0;
  uint8_t bus = ((num - 1) * 8) + 80;

  ret += pal_add_i2c_device((bus + 2), LM75_1_ADDR, "tmp75");
  ret += pal_add_i2c_device((bus + 3), LM75_2_ADDR, "tmp75");
  ret += pal_add_i2c_device((bus + 4), ADM1278_ADDR, "adm1278");

  return ret;
}

static int
pim_driver_del(uint8_t num) {
  int ret = 0;
  uint8_t bus = ((num - 1) * 8) + 80;

  ret += pal_del_i2c_device((bus + 2), LM75_1_ADDR);
  ret += pal_del_i2c_device((bus + 3), LM75_2_ADDR);
  ret += pal_del_i2c_device((bus + 4), ADM1278_ADDR);

  return ret;
}

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused){
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        syslog(LOG_WARNING, "SCM Insertion\n");

        /* Setup TH3 PCI-e repeater */
        run_command("/usr/local/bin/setup_pcie_repeater.sh th3 write");
        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          syslog(LOG_WARNING, "SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        syslog(LOG_WARNING, "SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
      sleep(1);
  }
  return 0;
}

// Thread for monitoring pim plug
static void *
pim_monitor_handler(void *unused){
  uint8_t fru;
  uint8_t num;
  uint8_t ret;
  uint8_t prsnt = 0;
  uint8_t prsnt_ori = 0;
  uint8_t curr_state = 0x00;
  uint8_t pim_type;
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t interval[10];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  while (1) {
    for(fru = FRU_PIM1; fru <= FRU_PIM8; fru++){
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* FRU_PIM1 = 3, FRU_PIM2 = 4, ...., FRU_PIM8 = 10 */
      /* Get original prsnt state PIM1 @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
      num = fru - 2;
      prsnt_ori = GETBIT(curr_state, (num - 1));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          ret = pim_driver_add(num);
          if(ret){
            syslog(LOG_WARNING, "PIM %d is not ready "
                                  "or sensor cannot be mounted.", num);
          }

          pim_type = pal_get_pim_type(fru);
          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, "16q")) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
            } else if (pim_type == PIM_TYPE_4DD) {
              if (!pal_set_pim_type_to_file(fru, "4dd")) {
                syslog(LOG_INFO, "PIM %d type is 4DD", num);
                pim_type_old[num] = PIM_TYPE_4DD;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 4DD failed", num);
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, "none")) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, DOMFPGA get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
              }
            }
            pal_set_pim_thresh(fru);
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged.", num);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          pal_clear_thresh_value(fru);
          ret = pim_driver_del(num);
          if (pal_set_pim_type_to_file(fru, "unplug")) {
            syslog(LOG_WARNING,
                    "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
        }
        /* Set PIM1 prsnt state @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 1))
                           : CLEARBIT(curr_state, (num - 1));
      }
      /* Set PIM number into DOM FPGA 0x03 register to control LED stream. */
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        if (interval[num] == 0) {
          interval[num] = INTERVAL_MAX;
          pal_set_pim_sts_led(fru);
        } else {
          interval[num]--;
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return 0;
}

void
exithandler(int signum) {
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
  exit(0);
}

// Thread for monitoring sim LED
static void *
simLED_monitor_handler(void *unused) {
  int brd_rev;
  uint8_t sys_ug = 0, fan_ug = 0, psu_ug = 0, smb_ug = 0;
  uint8_t interval[4];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  pal_get_board_rev(&brd_rev);
  init_led();
  while(1) {
    sleep(1);
    pal_mon_fw_upgrade(brd_rev, &sys_ug, &fan_ug, &psu_ug, &smb_ug);
    if (sys_ug == 0) {
      if (interval[0] == 0) {
        interval[0] = INTERVAL_MAX;
        set_sys_led(brd_rev);
      } else {
        interval[0]--;
      }
    }
    if (fan_ug == 0) {
      if (interval[1] == 0) {
        interval[1] = INTERVAL_MAX;
        set_fan_led(brd_rev);
      } else {
        interval[1]--;
      }
    }
    if (psu_ug == 0) {
      if (interval[2] == 0) {
        interval[2] = INTERVAL_MAX;
        set_psu_led(brd_rev);
      } else {
        interval[2]--;
      }
    }
    if (smb_ug == 0) {
      if (interval[3] == 0) {
        interval[3] = INTERVAL_MAX;
        set_smb_led(brd_rev);
      } else {
        interval[3]--;
      }
    }
  }
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t lpc;

  while (1) {
 
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);

    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        syslog(LOG_WARNING, "Debug Card Extraction\n");
      } else {
        // Debug Card was inserted
        syslog(LOG_WARNING, "Debug Card Insertion\n");
      }
    }

    // If Debug Card is present
    if (curr) {

      // Enable POST codes for scm slot
      ret = pal_post_enable(IPMB_BUS);
      if (ret) {
        goto debug_card_done;
      }

      // Get last post code and display it
      ret = pal_post_get_last(IPMB_BUS, &lpc);
      if (ret) {
        goto debug_card_done;
      }

      ret = pal_post_handle(IPMB_BUS, lpc);
      if (ret) {
        goto debug_card_out;
      }
    }

debug_card_done:
    prev = curr;
debug_card_out:
    if (curr == 1)
      msleep(500);
    else
      sleep(1);
  }

  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_scm_monitor;
  pthread_t tid_pim_monitor;
  pthread_t tid_debug_card;
  pthread_t tid_simLED_monitor;
  int rc;
  int pid_file;
  int brd_rev;
  signal(SIGTERM, exithandler);
  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pal_get_board_rev(&brd_rev)) {
    syslog(LOG_WARNING, "Get board revision failed\n");
    exit(-1);
  }

  if (pthread_create(&tid_scm_monitor, NULL, scm_monitor_handler, NULL) != 0) {
    syslog(LOG_WARNING, "pthread_create for scm monitor error\n");
    exit(1);
  }
  
  if (pthread_create(&tid_pim_monitor, NULL, pim_monitor_handler, NULL) != 0) {
    syslog(LOG_WARNING, "pthread_create for pim monitor error\n");
    exit(1);
  }
  
  if (pthread_create(&tid_simLED_monitor, NULL, simLED_monitor_handler, NULL) 
	  != 0) {
    syslog(LOG_WARNING, "pthread_create for simLED monitor error\n");
    exit(1);
  }

  if (brd_rev != BOARD_REV_EVTA) {
    if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) != 0) {
        syslog(LOG_WARNING, "pthread_create for debug card error\n");
        exit(1);
    }
    pthread_join(tid_debug_card, NULL);
  }

  pthread_join(tid_scm_monitor, NULL);
  pthread_join(tid_pim_monitor, NULL);
  pthread_join(tid_simLED_monitor, NULL);

  return 0;
}
