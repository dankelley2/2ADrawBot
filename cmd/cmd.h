/*
  cmd.h - Library for a string buffer with enQueue and deQueue methods.
*/
#ifndef cmd_h
#define cmd_h

#include "Arduino.h"

class CommandBuffer
{
  public:
    CommandBuffer();
    bool available();
    void deQueue(String & cmd0, String & cmd1, String & cmd2);
    bool enQueue(String cmd0, String cmd1, String cmd2);
    bool canEnQueue();
  private:
    String _buffer[10][3];
    unsigned int _count;
};

#endif
