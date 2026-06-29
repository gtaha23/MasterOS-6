#include "shell.h"
#include "vga.h"
#include "stdlib/stdio.h"
#include "fat.h"

// ─── Buffer ───

static char shell_buf[SHELL_BUFFER_SIZE];
static int  shell_len = 0;

// ─── String helpers ───

static int k_strlen(const char* s) {
    int i = 0; while (s[i]) i++; return i;
}

static int k_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static const char* k_arg(const char* cmd) {
    while (*cmd && *cmd != ' ') cmd++;
    if (*cmd == ' ' && *(cmd+1)) return cmd + 1;
    return 0;
}

static void k_first_word(const char* src, char* dst, int n) {
    int i = 0;
    while (src[i] && src[i] != ' ' && i < n - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void print_uint(uint32_t n) {
    if (n == 0) { print("0"); return; }
    char tmp[12]; int i = 0;
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) {
        char s[2] = { tmp[j], '\0' };
        print(s);
    }
}

// ─── Commands ───

static void cmd_help() {
    print("MasterOS Shell Commands:\r\n");
    print("  help       - Show this help\r\n");
    print("  cls        - Clear the screen\r\n");
    print("  echo <msg> - Print a message\r\n");
    print("  ver        - Show OS version\r\n");
    print("  dir        - List files on disk\r\n");
    print("  type <file>- Print file contents\r\n");
    print("  halt       - Halt the system\r\n");
}

static void cmd_cls()              { Reset(); }
static void cmd_ver()              { print("MasterOS v0.6.3 Kemal\r\n"); }

static void cmd_echo(const char* arg) {
    if (arg) { print(arg); print("\r\n"); }
    else      { print("\r\n"); }
}

static void cmd_halt() {
    print("System halted. You can power off now.\r\n");
    for (;;) { __asm__ volatile ("hlt"); }
}

static void cmd_dir() {
    Fat12Entry entries[FAT_MAX_FILES];
    int count = fat_list(entries, FAT_MAX_FILES);

    if (count < 0) { print("Error: could not read disk.\r\n"); return; }
    if (count == 0) { print("No files found.\r\n"); return; }

    print("Directory listing:\r\n");
    for (int i = 0; i < count; i++) {
        print("  ");
        print(entries[i].name);
        print("  (");
        print_uint(entries[i].size);
        print(" bytes)\r\n");
    }
    print_uint(count);
    print(" file(s).\r\n");
}

static uint8_t file_buf[32 * 1024];

static void cmd_type(const char* arg) {
    if (!arg) { print("Usage: type <filename>\r\n"); return; }

    int bytes = fat_read(arg, file_buf, sizeof(file_buf));
    if (bytes < 0) {
        print("File not found: ");
        print(arg);
        print("\r\n");
        return;
    }

    for (int i = 0; i < bytes; i++) {
        if (file_buf[i] == '\r') continue;
        if (file_buf[i] == '\n') { print("\r\n"); continue; }
        char tmp[2] = { (char)file_buf[i], '\0' };
        print(tmp);
    }
    print("\r\n");
}

// ─── Dispatcher ───

void shell_execute(const char* cmd) {
    if (!cmd || cmd[0] == '\0') return;

    char verb[32];
    k_first_word(cmd, verb, 32);
    const char* arg = k_arg(cmd);

    if      (k_strcmp(verb, "help") == 0) cmd_help();
    else if (k_strcmp(verb, "cls")  == 0) cmd_cls();
    else if (k_strcmp(verb, "echo") == 0) cmd_echo(arg);
    else if (k_strcmp(verb, "ver")  == 0) cmd_ver();
    else if (k_strcmp(verb, "dir")  == 0) cmd_dir();
    else if (k_strcmp(verb, "type") == 0) cmd_type(arg);
    else if (k_strcmp(verb, "halt") == 0) cmd_halt();
    else {
        print("Unknown command: ");
        print(verb);
        print("\r\n");
        print("Type 'help' for a list of commands.\r\n");
    }
}

// ─── Input handler ───

void shell_handle_char(char c) {
    if (c == '\n') {
        shell_buf[shell_len] = '\0';
        print("\r\n");
        shell_execute(shell_buf);
        shell_len = 0;
        print(">");
    } else if (c == '\b') {
        if (shell_len > 0) { shell_len--; print("\b"); }
    } else {
        if (shell_len < SHELL_BUFFER_SIZE - 1) {
            shell_buf[shell_len++] = c;
            char tmp[2] = { c, '\0' };
            print(tmp);
        }
    }
}

// ─── Init ────

void shell_init() {
    shell_len = 0;
    shell_buf[0] = '\0';
}
