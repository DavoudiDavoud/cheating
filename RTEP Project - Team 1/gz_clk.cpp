/*
 * gz_clk.c
 * 
 * Copyright 2012  campbellsan
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */
#include "gz_clk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <bcm2835.h>

// Address constants.
const uint32_t CM_CTL = 0x0; // Control register offset.
const uint32_t CM_DIV = 0x1; // Division register offset.
const uint32_t CM_GP0 = 0x1C; // General Purpose Clock 0.
const uint32_t CM_GP1 = 0x1E; // General Purpose Clock 1.
const uint32_t CM_GP2 = 0x20; // General Purpose Clock 2.
// Field constants.
const uint32_t CM_PASSWD = 0xFF << 24; // Clock Manager password mask.
const uint32_t CM_PASSWD_VAL = 0x5A << 24; // Clock Manager password.
const uint32_t CM_CTL_BUSY = 1 << 7; // Clock busy flag.
const uint32_t CM_CTL_ENAB = 1 << 4; // Clock enable flag.
const uint32_t CM_CTL_SRC = 0xF; // Clock source mask.

int gz_clock_ena(int speed, int divisor) {
  int speed_id = 6;
  if (speed < GZ_CLK_5MHz || speed > GZ_CLK_125MHz) {
    printf("gz_clock_ena: Unsupported clock speed selected.\n");
    printf("Supported speeds: GZ_CLK_5MHz (0) and GZ_CLK_125MHz (1).");
    exit(-1);
  }
  if (speed == 0) {
    speed_id = 1;
  }
  if (divisor < 2) {
    printf("gz_clock_ena: Minimum divisor value is 2.");
    exit(-1);
  }
  if (divisor > 0xfff) {
    printf("gz_clock_ena: Maximum divisor value is %d.", 0xfff);
    exit(-1);
  }
  if (bcm2835_init() !=1) {
    printf("gz_clock_ena: Failed to initialize I/O\n");
    exit(-1);
  }
  usleep(5);
  bcm2835_gpio_fsel(RPI_GPIO_P1_07, BCM2835_GPIO_FSEL_ALT0);

  // Select target clock.
  volatile uint32_t * clk_regs = bcm2835_clk + CM_GP0;
  // To prevent "glitches" modifications must use the current register state.
  uint32_t statectl = *(clk_regs + CM_CTL);
  statectl = (statectl & (~CM_PASSWD)) | CM_PASSWD_VAL;
  statectl&= ~CM_CTL_ENAB;
  // Shut down clock, wait until not busy to prevent "glitches".
  *(clk_regs + CM_CTL) = statectl;
  while (*(clk_regs + CM_CTL) & CM_CTL_BUSY) {}
  // Update the divisor register and start the clock with new settings.
  *(clk_regs + CM_DIV) = 0x5A002000 | (divisor << 12);
  *(clk_regs + CM_CTL) = 0x5A000010 | (speed_id & CM_CTL_SRC);
  return 0;
  
}

int gz_clock_dis() {
  if (bcm2835_init() !=1) {
    printf("gz_clock_dis: Failed to initialize I/O\n");
    exit(-1);
  }
  bcm2835_gpio_fsel(RPI_GPIO_P1_07, BCM2835_GPIO_FSEL_INPT);
  return 0;
}
