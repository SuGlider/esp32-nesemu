#ifdef _WIN32
#include <stdint.h>
#else
#include <Arduino.h>
#endif

#include "console.h"
#include "nofrendo-esp32/lcd.h"

Console console;
//void LCD_Write(int x, int y, const char* text, size_t len);

void Console::Clear()
{
    LCD_Fill(0, 0, LCD_W, LCD_H, m_crTextBk);
}

//void Console::SetTextColor(uint16_t crText, uint16_t crBk)
//{
//    m_crText = crText;
//    m_crTextBk = crBk;
//    LCD_SetTextColor(crText, crBk);
//}

void Console::Outputln(const char* szText)
{
    if (m_y >= LCD_W) // 横向
    {
        Scroll(0, -16);
        m_y -= 16;
    }
    LCD_Write(m_x, m_y, szText, -1);
    m_y += 16;
}

//void Console::DrawWindow(int x, int y, int w, int h)
//{
//    char buffer[LCD_H / 8 + 1] = {0xc9, 0xcd, 0xbb};
//    const char vert[] = { 0xba, 0 };
//    buffer[0] = 0xc9;
//    buffer[w - 1] = 0xbb;
//    for (int i = 1; i < w - 1; i++)
//    {
//        buffer[i] = 0xcd;
//    }
//    LCD_WriteASCII(x * 8, y * 16, buffer, w);
//    for (int i = 1; i < h - 1; i++)
//    {
//        LCD_WriteASCII(x * 8, (i + y) * 16, vert, 1);
//        LCD_WriteASCII((x + w - 1) * 8, (i + y) * 16, vert, 1);
//    }
//
//    buffer[0] = 0xc8;
//    buffer[w - 1] = 0xbc;
//    LCD_WriteASCII(x * 8, (y + h - 1) * 16, buffer, w);
//}


void Console::Scroll(int16_t offsetX, int16_t offsetY)
{
    LCD_Scroll(offsetY, offsetX); // 交换两个轴
}

