/*
  cmd.cpp - Library for a string buffer with enQueue and deQueue methods.
*/

#include "Arduino.h"
#include "cmd.h"

CommandBuffer::CommandBuffer()
{
  _count = 0;
}

bool CommandBuffer::available()
{
  return (_count > 0);
}

void CommandBuffer::deQueue(String & cmd0, String & cmd1, String & cmd2)
{
  cmd0 = _buffer[0][0];
  cmd1 = _buffer[0][1];
  cmd2 = _buffer[0][2];

  _count--;
  
  for (int i = 0; i < _count; i++){
    _buffer[i][0] = _buffer[i+1][0];
    _buffer[i][1] = _buffer[i+1][1];
    _buffer[i][2] = _buffer[i+1][2];
  }
}

bool CommandBuffer::enQueue(String cmd0, String cmd1, String cmd2)
{
  if (_count < 10){
    _buffer[_count][0] = cmd0;
    _buffer[_count][1] = cmd1;
    _buffer[_count][2] = cmd2;
    _count++;
    return true;
  }
    return false;
}

bool CommandBuffer::canEnQueue()
{
  if (_count < 10){
    return true;
  }
    return false;
}

