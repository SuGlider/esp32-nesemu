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

static const char *TAG = "OSD";

#define LOBYTE(u16)     ((uint8_t)(((uint16_t)(u16)) & 0xff))
#define HIBYTE(u16)     ((uint8_t)((((uint16_t)(u16))>>8) & 0xff))

//Nes stuff wants to define this as well...
#undef false
#undef true
#undef bool

#include "esp_log.h"
#include "noftypes.h"
#include "nofconfig.h"
#include "nofrendo.h"
#include "bitmap.h"
#include "osd.h"
#include "event.h"
#include "nesinput.h"
#include "bsp/esp-bsp.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#define APP_DEFAULT_WIDTH  256
#define APP_DEFAULT_HEIGHT 240
#define APP_DEFAULT_ZOOM   256

typedef struct {
   int16_t x;              /* Mouse X coordinate */
   int16_t y;              /* Mouse Y coordinate */
   bool left_button;       /* Mouse left button state */
} app_mouse_t;
typedef struct {
   uint32_t last_key;
   bool     pressed;
} app_kb_t;

typedef struct {
    hid_host_device_handle_t hid_device_handle;
    hid_host_driver_event_t event;
    void *arg;
} app_usb_hid_event_t;

typedef struct
{
    uint8_t reserved1;
    uint8_t reserved2;
    
    bool    btn_select:1;
    bool    btn_res1:1;
    bool    btn_res2:1;
    bool    btn_start:1;
    bool    btn_up:1;
    bool    btn_right:1;
    bool    btn_down:1;
    bool    btn_left:1;
    
    bool    btn_res3:4;
    bool    btn_triangle:1;
    bool    btn_circle:1;
    bool    btn_cross:1;
    bool    btn_square:1;
} __attribute__((packed)) gamepad_t;

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

/* USB functions */
static void app_hid_init(void);
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

/* Video variables */
static uint16 colorPalette[256];
static bitmap_t *vidBitmap = NULL;
static lv_obj_t * lvgl_video_canvas = NULL;
static uint8_t * vid_buffer = NULL;
static uint8_t * vid_buffer_tmp = NULL;
static lv_img_dsc_t vid_tmp_img;

/* USB variables */
static QueueHandle_t hid_queue = NULL;
static app_kb_t app_kb;
static app_mouse_t app_mouse;
static gamepad_t app_gamepad;
static gamepad_t app_gamepad_tmp;
/*******************************************************************************
* Public API functions
*******************************************************************************/

void osd_getvideoinfo(vidinfo_t *info)
{
   info->default_width = APP_DEFAULT_WIDTH;
   info->default_height = APP_DEFAULT_HEIGHT;
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
   uint32_t buf_size = APP_DEFAULT_WIDTH * APP_DEFAULT_HEIGHT * sizeof(uint16_t);
   vid_buffer = heap_caps_malloc(buf_size, MALLOC_CAP_DEFAULT);
   assert(vid_buffer);
   memset(vid_buffer, 0x00, buf_size);
   //vid_buffer_tmp = heap_caps_malloc(buf_size, MALLOC_CAP_DEFAULT);
   //assert(vid_buffer_tmp);
   //memset(vid_buffer_tmp, 0x00, buf_size);

   /*vid_tmp_img.data = (void *)vid_buffer_tmp;
   vid_tmp_img.header.cf = LV_IMG_CF_TRUE_COLOR;
   vid_tmp_img.header.w = APP_DEFAULT_WIDTH;
   vid_tmp_img.header.h = APP_DEFAULT_HEIGHT;*/

   app_hid_init();

   bsp_display_lock(0);
   lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
   lvgl_video_canvas = lv_canvas_create(lv_scr_act());
   lv_canvas_set_buffer(lvgl_video_canvas, vid_buffer, APP_DEFAULT_WIDTH, APP_DEFAULT_HEIGHT, LV_IMG_CF_TRUE_COLOR);
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
	event_t evh = NULL;
   int pressed = 0;

   if(app_gamepad_tmp.btn_select != app_gamepad.btn_select)
   {
      pressed = (app_gamepad.btn_select ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_select);
      app_gamepad_tmp.btn_select = app_gamepad.btn_select;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_start != app_gamepad.btn_start)
   {
      pressed = (app_gamepad.btn_start ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_start);
      app_gamepad_tmp.btn_start = app_gamepad.btn_start;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_up != app_gamepad.btn_up)
   {
      pressed = (app_gamepad.btn_up ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_up);
      app_gamepad_tmp.btn_up = app_gamepad.btn_up;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_down != app_gamepad.btn_down)
   {
      pressed = (app_gamepad.btn_down ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_down);
      app_gamepad_tmp.btn_down = app_gamepad.btn_down;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_right != app_gamepad.btn_right)
   {
      pressed = (app_gamepad.btn_right ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_right);
      app_gamepad_tmp.btn_right = app_gamepad.btn_right;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_left != app_gamepad.btn_left)
   {
      pressed = (app_gamepad.btn_left ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_left);
      app_gamepad_tmp.btn_left = app_gamepad.btn_left;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_cross != app_gamepad.btn_cross)
   {
      pressed = (app_gamepad.btn_cross ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_a);
      app_gamepad_tmp.btn_cross = app_gamepad.btn_cross;

      if(evh) {
         evh(pressed);
      }
   }
   
   if(app_gamepad_tmp.btn_circle != app_gamepad.btn_circle)
   {
      pressed = (app_gamepad.btn_circle ? INP_STATE_MAKE : INP_STATE_BREAK);
      evh = event_get(event_joypad1_b);
      app_gamepad_tmp.btn_circle = app_gamepad.btn_circle;

      if(evh) {
         evh(pressed);
      }
   }
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

   for(int x=0; x<APP_DEFAULT_WIDTH; x++)
   {
      for(int y=0; y<APP_DEFAULT_HEIGHT; y++)
      {
         vid_buffer[(x*APP_DEFAULT_HEIGHT*2) + (y*2)] = HIBYTE(colorPalette[color]);
         vid_buffer[(x*APP_DEFAULT_HEIGHT*2) + (y*2)+1] = LOBYTE(colorPalette[color]);
      }
   }

   lv_obj_invalidate(lvgl_video_canvas);
   bsp_display_unlock();
}

/* acquire the directbuffer for writing */
static bitmap_t *app_osd_video_lock_write(void)
{
   vidBitmap = bmp_create(APP_DEFAULT_WIDTH, APP_DEFAULT_HEIGHT, APP_DEFAULT_WIDTH*sizeof(uint16));
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

   for(int x=0; x<APP_DEFAULT_WIDTH; x++)
   {
      for(int y=0; y<APP_DEFAULT_HEIGHT; y++)
      {
         vid_buffer[(x*APP_DEFAULT_HEIGHT*2) + (y*2)] = HIBYTE(colorPalette[data[0]]);
         vid_buffer[(x*APP_DEFAULT_HEIGHT*2) + (y*2)+1] = LOBYTE(colorPalette[data[0]]);
         data++;
      }
   }

   bsp_display_lock(0);
   //lv_canvas_transform(lvgl_video_canvas, &vid_tmp_img, 0, APP_DEFAULT_ZOOM, 0, 0, 0, 0, true);
   lv_obj_invalidate(lvgl_video_canvas);
   bsp_display_unlock();
}

static char usb_hid_get_keyboard_char(uint8_t key, uint8_t shift)
{
    char ret_key = 0;

    const uint8_t keycode2ascii [57][2] = {
        {0, 0}, /* HID_KEY_NO_PRESS        */
        {0, 0}, /* HID_KEY_ROLLOVER        */
        {0, 0}, /* HID_KEY_POST_FAIL       */
        {0, 0}, /* HID_KEY_ERROR_UNDEFINED */
        {'a', 'A'}, /* HID_KEY_A               */
        {'b', 'B'}, /* HID_KEY_B               */
        {'c', 'C'}, /* HID_KEY_C               */
        {'d', 'D'}, /* HID_KEY_D               */
        {'e', 'E'}, /* HID_KEY_E               */
        {'f', 'F'}, /* HID_KEY_F               */
        {'g', 'G'}, /* HID_KEY_G               */
        {'h', 'H'}, /* HID_KEY_H               */
        {'i', 'I'}, /* HID_KEY_I               */
        {'j', 'J'}, /* HID_KEY_J               */
        {'k', 'K'}, /* HID_KEY_K               */
        {'l', 'L'}, /* HID_KEY_L               */
        {'m', 'M'}, /* HID_KEY_M               */
        {'n', 'N'}, /* HID_KEY_N               */
        {'o', 'O'}, /* HID_KEY_O               */
        {'p', 'P'}, /* HID_KEY_P               */
        {'q', 'Q'}, /* HID_KEY_Q               */
        {'r', 'R'}, /* HID_KEY_R               */
        {'s', 'S'}, /* HID_KEY_S               */
        {'t', 'T'}, /* HID_KEY_T               */
        {'u', 'U'}, /* HID_KEY_U               */
        {'v', 'V'}, /* HID_KEY_V               */
        {'w', 'W'}, /* HID_KEY_W               */
        {'x', 'X'}, /* HID_KEY_X               */
        {'y', 'Y'}, /* HID_KEY_Y               */
        {'z', 'Z'}, /* HID_KEY_Z               */
        {'1', '!'}, /* HID_KEY_1               */
        {'2', '@'}, /* HID_KEY_2               */
        {'3', '#'}, /* HID_KEY_3               */
        {'4', '$'}, /* HID_KEY_4               */
        {'5', '%'}, /* HID_KEY_5               */
        {'6', '^'}, /* HID_KEY_6               */
        {'7', '&'}, /* HID_KEY_7               */
        {'8', '*'}, /* HID_KEY_8               */
        {'9', '('}, /* HID_KEY_9               */
        {'0', ')'}, /* HID_KEY_0               */
        {'\r', '\r'}, /* HID_KEY_ENTER           */
        {0, 0}, /* HID_KEY_ESC             */
        {'\b', 0}, /* HID_KEY_DEL             */
        {0, 0}, /* HID_KEY_TAB             */
        {' ', ' '}, /* HID_KEY_SPACE           */
        {'-', '_'}, /* HID_KEY_MINUS           */
        {'=', '+'}, /* HID_KEY_EQUAL           */
        {'[', '{'}, /* HID_KEY_OPEN_BRACKET    */
        {']', '}'}, /* HID_KEY_CLOSE_BRACKET   */
        {'\\', '|'}, /* HID_KEY_BACK_SLASH      */
        {'\\', '|'}, /* HID_KEY_SHARP           */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
        {';', ':'}, /* HID_KEY_COLON           */
        {'\'', '"'}, /* HID_KEY_QUOTE           */
        {'`', '~'}, /* HID_KEY_TILDE           */
        {',', '<'}, /* HID_KEY_LESS            */
        {'.', '>'}, /* HID_KEY_GREATER         */
        {'/', '?'} /* HID_KEY_SLASH           */
    };

    if (shift > 1) {
        shift = 1;
    }

    if ((key >= HID_KEY_A) && (key <= HID_KEY_SLASH)) {
        ret_key = keycode2ascii[key][shift];
    }

    return ret_key;
}

static void app_usb_hid_host_interface_callback(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event, void *arg)
{
   hid_host_dev_params_t *dev = hid_host_device_get_params(hid_device_handle);
   uint8_t *data = NULL;
   unsigned int data_length = 0;

   assert(dev != NULL);

   switch (event) {
   case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
      data = hid_host_device_get_data(hid_device_handle, &data_length);
      if (dev->proto == HID_PROTOCOL_KEYBOARD) {
         hid_keyboard_input_report_boot_t *keyboard = (hid_keyboard_input_report_boot_t *)data;
         if (data_length < sizeof(hid_keyboard_input_report_boot_t)) {
               return;
         }
         for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {
               if (keyboard->key[i] > HID_KEY_ERROR_UNDEFINED) {
                  char key = 0;

                  /* LVGL special keys */
                  if (keyboard->key[i] == HID_KEY_TAB) {
                     if ((keyboard->modifier.left_shift || keyboard->modifier.right_shift)) {
                           key = LV_KEY_PREV;
                     } else {
                           key = LV_KEY_NEXT;
                     }
                  } else if (keyboard->key[i] == HID_KEY_RIGHT) {
                     key = LV_KEY_RIGHT;
                  } else if (keyboard->key[i] == HID_KEY_LEFT) {
                     key = LV_KEY_LEFT;
                  } else if (keyboard->key[i] == HID_KEY_DOWN) {
                     key = LV_KEY_DOWN;
                  } else if (keyboard->key[i] == HID_KEY_UP) {
                     key = LV_KEY_UP;
                  } else if (keyboard->key[i] == HID_KEY_ENTER || keyboard->key[i] == HID_KEY_KEYPAD_ENTER) {
                     key = LV_KEY_ENTER;
                  } else if (keyboard->key[i] == HID_KEY_DELETE) {
                     key = LV_KEY_DEL;
                  } else if (keyboard->key[i] == HID_KEY_HOME) {
                     key = LV_KEY_HOME;
                  } else if (keyboard->key[i] == HID_KEY_END) {
                     key = LV_KEY_END;
                  } else {
                     /* Get ASCII char */
                     key = usb_hid_get_keyboard_char(keyboard->key[i], (keyboard->modifier.left_shift || keyboard->modifier.right_shift));
                  }

                  if (key == 0) {
                     ESP_LOGI(TAG, "Not recognized key: %c (%d)", keyboard->key[i], keyboard->key[i]);
                  }
                  app_kb.last_key = key;
                  app_kb.pressed = true;
               }
               else
               {
                  if(i == 0) {
                     app_kb.pressed = false;
                  }
               }
               
         }

      } else if (dev->proto == HID_PROTOCOL_MOUSE) {
         hid_mouse_input_report_boot_t *mouse = (hid_mouse_input_report_boot_t *)data;
         if (data_length < sizeof(hid_mouse_input_report_boot_t)) {
               break;
         }
         app_mouse.left_button = mouse->buttons.button1;
         app_mouse.x += mouse->x_displacement;
         app_mouse.y += mouse->y_displacement;
      } else {
         gamepad_t *gamepad = (gamepad_t*)data;
         if (data_length < 1) {
               break;
         }
         memcpy(&app_gamepad, gamepad, sizeof(gamepad_t));
         osd_getinput();
         //ESP_LOGW(TAG, "Gamepad... len: %d; 0x%02x 0x%02x 0x%02x 0x%02x ", data_length, data[0], data[1], data[2], data[3]);
      }
      break;
   case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
      break;
   case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
      hid_host_device_close(hid_device_handle);
      break;
   default:
      break;
   }
}

static void app_usb_hid_task(void *arg)
{
    hid_host_device_handle_t hid_device_handle = NULL;
    app_usb_hid_event_t msg;

    while (1) {
        if (xQueueReceive(hid_queue, &msg, pdMS_TO_TICKS(50))) {
            hid_device_handle = msg.hid_device_handle;
            hid_host_dev_params_t *dev = hid_host_device_get_params(hid_device_handle);

            assert(dev != NULL);

            switch (msg.event) {
            case HID_HOST_DRIVER_EVENT_CONNECTED:
               /* Handle mouse or keyboard */
               if (dev->proto == HID_PROTOCOL_NONE || dev->proto == HID_PROTOCOL_KEYBOARD || dev->proto == HID_PROTOCOL_MOUSE) {
                  const hid_host_device_config_t dev_config = {
                     .callback = app_usb_hid_host_interface_callback,
                  };

                  ESP_ERROR_CHECK( hid_host_device_open(hid_device_handle, &dev_config) );
                  ESP_ERROR_CHECK( hid_class_request_set_idle(hid_device_handle, 0, 0) );
                  ESP_ERROR_CHECK( hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT) );
                  ESP_ERROR_CHECK( hid_host_device_start(hid_device_handle) );
               } else {
                  ESP_LOGE(TAG, "Other HID device connected! Proto: %d", dev->proto);
               }
               break;
            default:
               ESP_LOGE(TAG, "Not handled HID event!");
               break;
            }
        }
    }

    xQueueReset(hid_queue);
    vQueueDelete(hid_queue);

    hid_host_uninstall();

    vTaskDelete(NULL);
}

static void app_usb_hid_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg)
{
    const app_usb_hid_event_t msg = {
        .hid_device_handle = hid_device_handle,
        .event = event,
        .arg = arg
    };

    xQueueSend(hid_queue, &msg, 0);
}

static void app_hid_init(void)
{
   esp_err_t ret;

   /* USB HID host driver config */
   const hid_host_driver_config_t hid_host_driver_config = {
      .create_background_task = true,
      .task_priority = 5,
      .stack_size = 4096,
      .core_id = 0,
      .callback = app_usb_hid_callback,
   };

   ret = hid_host_install(&hid_host_driver_config);
   if (ret != ESP_OK) {
      ESP_LOGE(TAG, "USB HID install failed!");
      assert(0);
   }

   hid_queue = xQueueCreate(10, sizeof(app_usb_hid_event_t));
   xTaskCreate(&app_usb_hid_task, "hid_task", 4 * 1024, NULL, 2, NULL);
}