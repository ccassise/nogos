#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define LOG_DEBUG(...) printf("[DEBUG] " __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__) 

#endif
