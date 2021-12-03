/*
 * Copyright 2021 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/librpitx-C *
 *
 * This is based on other projects:
 *    librpitx (https://github.com/F5OEO/librpitx
 *        Copyright (C) 2018  Evariste COURJAUD F5OEO
 *
 *    please contact their authors for more information.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <stdint.h>

#include "util.h"

static unsigned get_dt_ranges(const char *filename, unsigned offset) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    unsigned address = ~0;
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        unsigned char buf[4];
        fseek(fp, offset, SEEK_SET);
        if (fread(buf, 1, sizeof buf, fp) == sizeof buf)
            address = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0;
        fclose(fp);
    }
    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
    return address;
}

unsigned bcm_host_get_peripheral_address(void) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);
    if (address == 0)
        address = get_dt_ranges("/proc/device-tree/soc/ranges", 8);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
    return address == ~0u ? 0x20000000 : address;
}

unsigned bcm_host_get_sdram_address(void) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    unsigned address = get_dt_ranges("/proc/device-tree/axi/vc_mem/reg", 8);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
    return address == ~0u ? 0x40000000 : address;
}
