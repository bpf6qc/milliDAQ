#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdio.h>
#include <ctime>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <iostream>

int kbhit(void) {

  struct termios oldt, newt;
  int ch;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

#endif