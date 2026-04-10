#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

void keyboard_install(void);
bool keyboard_has_char(void);
char keyboard_getchar(void);

#endif
