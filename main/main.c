/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
 
#include "esp_system.h"
#include "nvs_flash.h"
#include "nofrendo.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "bsp/esp-bsp.h"

char *osd_getromdata() {
	char* romdata;
	const esp_partition_t* part;
	spi_flash_mmap_handle_t hrom;
	esp_err_t err;
	nvs_flash_init();
	part=esp_partition_find_first(0x40, 1, NULL);
	if (part==0) printf("Couldn't find rom part!\n");
	err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err!=ESP_OK) printf("Couldn't map rom part!\n");
	printf("Initialized. ROM@%p\n", romdata);
    return (char*)romdata;
}


/*esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}*/

int app_main(void)
{
    /* Initialize display and LVGL */
    lv_disp_t * display = bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();
    
    
	printf("NoFrendo start!\n");
	nofrendo_main(0, NULL);
	printf("NoFrendo died? WtF?\n");
	//asm("break.n 1");
    return 0;
}

