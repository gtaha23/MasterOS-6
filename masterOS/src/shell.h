#pragma once
#include "stdint.h"

#define SHELL_BUFFER_SIZE 256

void shell_init();
void shell_handle_char(char c);
void shell_execute(const char* cmd);
void cmd_oldfetch(const char* arg);
void print_uint(uint32_t n);
