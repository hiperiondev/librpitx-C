/*
 Copyright ????
 */

#ifndef CORE_RPI_H_
#define CORE_RPI_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned get_dt_ranges(const char *filename, unsigned offset);
unsigned bcm_host_get_peripheral_address(void);
unsigned bcm_host_get_sdram_address(void);

#ifdef __cplusplus
}
#endif

#endif
