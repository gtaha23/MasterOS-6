#pragma once

#define SHELL_BUFFER_SIZE 256

void shell_init();
void shell_handle_char(char c);
void shell_execute(const char* cmd);
