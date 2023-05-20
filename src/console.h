#ifndef _ARTINT_CONSOLE_H_
#define _ARTINT_CONSOLE_H_

#include <Artino_Console.h>

class Console : public Artino::Console
{
public:
  virtual void Clear() override;
  virtual void Outputln(const char* szText) override;
  virtual void Scroll(int16_t offsetX, int16_t offsetY) override;
};

#endif // _ARTINT_CONSOLE_H_