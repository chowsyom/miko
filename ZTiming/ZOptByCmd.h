#ifndef _ZOptByCmd_H_
#define _ZOptByCmd_H_

struct OptByCmd
{
  uint8_t time[2] = {0xFF, 0xFF};
  bool enable = false;
  bool isOn = false;
};
#endif