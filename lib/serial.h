#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "../config.h"

extern void serial_init();

extern void serial_string(char* s);

extern void serial_break();

#endif