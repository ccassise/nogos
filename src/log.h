#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define DEBUG(fmt, ...) fprintf(stderr, "[DEBUG] " fmt, ## __VA_ARGS__)

#endif
