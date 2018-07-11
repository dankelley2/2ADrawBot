/*
  cmd.h - Library for a string buffer with push pop methods.
*/
#ifndef cmd_h
#define cmd_h

#include "Arduino.h"

class CommandBuffer
{
  public:
    CommandBuffer();
    bool available();
    void pop(String & cmd0, String & cmd1, String & cmd2);
    bool push(String cmd0, String cmd1, String cmd2);
    bool canPush();
  private:
    String _buffer[10][3];
    unsigned int _count;
};

#endif
