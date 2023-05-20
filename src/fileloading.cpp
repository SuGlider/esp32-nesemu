#ifdef _WIN32
#include <Windows.h>
#include <shlwapi.h>
#define delay Sleep
#else
#include <Arduino.h>
#include <cstring>
#endif
#include <vector>
#include <string>
#include <fstream>
#include <Artino_Menu.h>
#include <Artino_Console.h>
#include "nofrendo-esp32\psxcontroller.h"
#include "console.h"
#include "osd.h"
#include "nofrendo-esp32\lcd.h"

void LCD_SetTextColor(uint16_t c, uint16_t bk);
extern Console console;

const uint8_t* GetDefaultRom();

#define RGB16(r, g, b) (((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3))
#define YELLOW RGB16(255, 255, 0)
#define BLUE RGB16(0, 0, 255)
#define WHITE RGB16(255, 255, 255)
#define BLACK RGB16(0, 0, 0)
//#define RGB32to16


struct MENU_ITEM
{
  std::string str;
};

class Menu : public Artino::Menu<MENU_ITEM>
{
  uint32_t m_dwPrevKey = psxReadInput();
public:
  Menu(const MENU_ITEM* items, uint16_t count, const Artino::RECT* lprc)
    : Artino::Menu<MENU_ITEM>(items, count, 16, lprc)
  {
  }

  Artino::MenuKey GetKey() override
  {
    uint32_t key = psxReadInput();
    uint32_t dwPrevKey = m_dwPrevKey;
    m_dwPrevKey = key;

    if ((dwPrevKey & (1 << KEYSHIFT_UP)) && (key & (1 << KEYSHIFT_UP)) == 0)
    {
      return Artino::MenuKey_Up;
    }
    else if ((dwPrevKey & (1 << KEYSHIFT_DOWN)) && (key & (1 << KEYSHIFT_DOWN)) == 0)
    {
      return Artino::MenuKey_Down;
    }
    else if ((dwPrevKey & (1 << KEYSHIFT_RIGHT)) && (key & (1 << KEYSHIFT_RIGHT)) == 0)
    {
      return Artino::MenuKey_Confirm;
    }

    return Artino::MenuKey_None;
  }

  //void OnBeginDrawItem() override
  //{
  //  LCD_Fill(0, 0, LCD_H, LCD_W, BLUE);
  //}

  void OnDrawItem(int16_t x, int16_t y, const MENU_ITEM* pItem, bool selected) override
  {
    if (selected)
    {
      LCD_SetTextColor(BLUE, YELLOW);
    }
    else
    {
      LCD_SetTextColor(YELLOW, BLUE);
    }
    const int len = 320 / 8;
    char buffer[len];
    size_t n = pItem->str.length();
    LCD_Write(x, y, pItem->str.c_str(), -1);
    /*if (n < len)
    {
      memset(buffer, 0x20, len - n);
      buffer[len - n] = '\0';
      LCD_Write(x + n * 8, y, buffer, -1);
    }//*/
  }
};

#ifdef _WIN32

class MyFile : public OSDFile
{
  friend OSDFile* OpenDir(const char* strDir);
  HANDLE hFind = NULL;
  WIN32_FIND_DATAA wfd = {0};

  OSDFile* OpenNextFile() override
  {
    BOOL result = FindNextFileA(hFind, &wfd);
    if (!result)
    {
      FindClose(hFind);
      hFind = INVALID_HANDLE_VALUE;
      memset(&wfd, 0, sizeof(WIN32_FIND_DATAA));
      return NULL;
    }
    return this;
  }

  const char* name() const override
  {
    return wfd.cFileName;
  }

  const unsigned char* ReadFile(const char* szFile)
  {
    std::fstream file(szFile, std::ios::in | std::ios::binary);
    //file.open(strFile.c_str(), std::ios_base::in);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    char* pData = nullptr;
    if (size)
    {
      pData = new char[size];
      file.read(pData, size);
    }
    file.close();
    return reinterpret_cast<unsigned char*>(pData);
  }
};

MyFile file;

OSDFile* OpenDir(const char* strDir)
{
  if (file.hFind)
  {
    FindClose(file.hFind);
  }

  char buffer[MAX_PATH];
  //memset(&file, 0, sizeof(MyFile));
  GetCurrentDirectoryA(MAX_PATH, buffer);
  PathCombineA(buffer, buffer, "*");

  file.hFind = FindFirstFileA(buffer, &file.wfd);
  return &file;
}



#else

#endif


bool IsNESFilename(const std::string& strFilename)
{
  auto pos = strFilename.rfind('.');
  if (pos == std::string::npos)
  {
    return false;
  }
  return strFilename.compare(pos, 4, ".NES") == 0;
}


//
const unsigned char* osd_getromdata()
{
#if 1
  console.SetTextColor(YELLOW, BLUE);
  console.Clear();
  console.Outputln("osd_getromdata");
  delay(1000);
  //console.DrawWindow(0, 0, 40, 15);
  OSDFile* root = OpenDir("/");

  std::vector<MENU_ITEM> files;
  if (root)
  {
    OSDFile* file = root->OpenNextFile();
    while (file && files.size() < 10)
    {
      MENU_ITEM item;
      item.str = file->name();
      if (IsNESFilename(item.str))
      {
        files.push_back(item);
        TRACE(file->name());
      }
      file = root->OpenNextFile();
    }
  }

  TRACE("generate file list [OK]");

 
  if (files.empty())
  {
    TRACE("empty file list");
    console.Outputln("empty file list");
    while (1);
  }

  Artino::RECT rect = { 0, 0, LCD_H, LCD_W };
  Menu menu(&files.front(), files.size(), &rect);

  TRACE("enter menu loop");
  int select = menu.Loop();

  TRACE("exit menu loop");
  console.SetTextColor(WHITE, BLACK);
  console.Clear();
  return root->ReadFile(files[select].str.c_str());

#else
  return GetDefaultRom();
#endif
}