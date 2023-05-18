/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*******************************************************************************
* Includes
*******************************************************************************/
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <string.h>

#define LOBYTE(u16)     ((uint8_t)(((uint16_t)(u16)) & 0xff))
#define HIBYTE(u16)     ((uint8_t)((((uint16_t)(u16))>>8) & 0xff))

//Nes stuff wants to define this as well...
#undef false
#undef true
#undef bool

#include "noftypes.h"
#include "nofconfig.h"
#include "nofrendo.h"
#include "bitmap.h"
#include "osd.h"
#include "bsp/esp-bsp.h"


/*******************************************************************************
* Function definitions
*******************************************************************************/
/* Video functions */
static int app_osd_video_init(int width, int height);
static void app_osd_video_shutdown(void);
static int app_osd_video_set_mode(int width, int height);
static void app_osd_video_set_palette(rgb_t *palette);
static void app_osd_video_clear(uint8 color);
static bitmap_t *app_osd_video_lock_write(void);
static void app_osd_video_free_write(int num_dirties, rect_t *dirty_rects);
static void app_osd_video_custom_blit(bitmap_t *primary, int num_dirties, rect_t *dirty_rects);

/* Audio functions */
/*******************************************************************************
* Local variables
*******************************************************************************/

static viddriver_t vid_driver = {
   .name = "Vid Driver",
   .init = app_osd_video_init,
   .shutdown = app_osd_video_shutdown,
   .set_mode = app_osd_video_set_mode,
   .set_palette = app_osd_video_set_palette,
   .clear = app_osd_video_clear,
   .lock_write = app_osd_video_lock_write,
   .free_write = app_osd_video_free_write,
   .custom_blit = app_osd_video_custom_blit,
   .invalidate = false,
};

static TimerHandle_t timer; 
static uint16 colorPalette[256];
static bitmap_t *vidBitmap = NULL;

static lv_obj_t * lvgl_video_canvas = NULL;
uint8_t * vid_buffer = NULL;

/*******************************************************************************
* Public API functions
*******************************************************************************/

void osd_getvideoinfo(vidinfo_t *info)
{
   info->default_width = 256;
   info->default_height = BSP_LCD_V_RES;
   info->driver = &vid_driver;
}

void osd_getsoundinfo(sndinfo_t *info)
{
}

void osd_setsound(void (*playfunc)(void *buffer, int size))
{
}

int osd_init(void)
{
   uint32_t buf_size = 256 * BSP_LCD_V_RES * sizeof(uint16_t);
   vid_buffer = heap_caps_malloc(buf_size, MALLOC_CAP_DEFAULT);
   assert(vid_buffer);
   memset(vid_buffer, 0, buf_size);

   bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
   lvgl_video_canvas = lv_canvas_create(lv_scr_act());
   lv_canvas_set_buffer(lvgl_video_canvas, vid_buffer, 256, BSP_LCD_V_RES, LV_IMG_CF_TRUE_COLOR);
   lv_obj_center(lvgl_video_canvas);
   bsp_display_unlock();

	return 0;
}

void osd_shutdown(void)
{
}

/* This is os-specific part of main() */
int osd_main(int argc, char *argv[])
{
   config.filename = "config.cfg";
   return main_loop("rom", system_autodetect);
}

int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
	printf("Timer install, freq=%d\n", frequency);
	timer=xTimerCreate("nes",configTICK_RATE_HZ/frequency, pdTRUE, NULL, func);
	xTimerStart(timer, 0);
   return 0;
}

/* File system interface */
void osd_fullname(char *fullname, const char *shortname)
{
   strncpy(fullname, shortname, PATH_MAX);
}

/* This gives filenames for storage of saves */
char *osd_newextension(char *string, char *ext)
{
   return string;
}

void osd_getinput(void)
{
}

void osd_getmouse(int *x, int *y, int *button)
{
}

/* This gives filenames for storage of PCX snapshots */
int osd_makesnapname(char *filename, int len)
{
   return -1;
}

/*******************************************************************************
* Private API functions
*******************************************************************************/

static int app_osd_video_init(int width, int height)
{
	return 0;
}

static void app_osd_video_shutdown(void)
{

}

/* set a video mode */
static int app_osd_video_set_mode(int width, int height)
{
   return 0;
}

/* copy nes palette over to hardware */
static void app_osd_video_set_palette(rgb_t *palette)
{
	uint16 c;

   for (int i = 0; i < 256; i++)
   {
      c=(palette[i].b>>3)+((palette[i].g>>2)<<5)+((palette[i].r>>3)<<11);
      colorPalette[i]=c;
   }
}

/* clear all frames to a particular color */
static void app_osd_video_clear(uint8 color)
{
   bsp_display_lock(0);

   for(int x=0; x<256; x++)
   {
      for(int y=0; y<BSP_LCD_V_RES; y++)
      {
         vid_buffer[(x*BSP_LCD_V_RES*2) + (y*2)] = HIBYTE(colorPalette[color]);
         vid_buffer[(x*BSP_LCD_V_RES*2) + (y*2)+1] = LOBYTE(colorPalette[color]);
      }
   }

   lv_obj_invalidate(lvgl_video_canvas);
   bsp_display_unlock();
}

/* acquire the directbuffer for writing */
static bitmap_t *app_osd_video_lock_write(void)
{
   vidBitmap = bmp_create(256, BSP_LCD_V_RES, 256*sizeof(uint16));
   return vidBitmap;
}

static void app_osd_video_free_write(int num_dirties, rect_t *dirty_rects)
{
   bmp_destroy(&vidBitmap);
   vidBitmap = NULL;
}

static void app_osd_video_custom_blit(bitmap_t *primary, int num_dirties, rect_t *dirty_rects)
{	
   const uint8_t * data = primary->data;
   bsp_display_lock(0);

   for(int x=0; x<256; x++)
   {
      for(int y=0; y<BSP_LCD_V_RES; y++)
      {
         vid_buffer[(x*BSP_LCD_V_RES*2) + (y*2)] = HIBYTE(colorPalette[data[0]]);
         vid_buffer[(x*BSP_LCD_V_RES*2) + (y*2)+1] = LOBYTE(colorPalette[data[0]]);
         data++;
      }
   }

   lv_obj_invalidate(lvgl_video_canvas);
   bsp_display_unlock();
}
