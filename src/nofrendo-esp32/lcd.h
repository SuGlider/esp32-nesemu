// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _DRIVER_SPI_LCD_H_
#define _DRIVER_SPI_LCD_H_
#include <stdint.h>

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

//#ifdef __cplusplus
//extern "C"
//{
//#endif

void LCD_Init();
void LCD_Display(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height, const uint8_t *data);
void LCD_SetTextColor(uint16_t c, uint16_t bk);
void LCD_Write(int x, int y, const char* text, size_t len);
void LCD_WriteASCII(int x, int y, const char* text, size_t len);
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_Scroll(int16_t offsetX, int16_t offsetY);


#define LCD_W 240
#define LCD_H 320

//#ifdef __cplusplus
//}
//#endif

#endif //  __SPI_H__
