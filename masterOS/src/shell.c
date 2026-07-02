#include "shell.h"
#include "vga.h"
#include "stdlib/stdio.h"
#include "fat.h"
#include "rtc.h"

extern uint32_t total_mem_kb;  // set in kernel.c at boot


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

// ─── Commands ────

static void cmd_help() {
    print("MasterOS Shell Commands:\r\n");
    print("  help       - Show this help\r\n");
    print("  cls        - Clear the screen\r\n");
    print("  echo <msg> - Print a message\r\n");
    print("  ver        - Show OS version\r\n");
    print("  dir        - List files on disk\r\n");
    print("  type <file>- Print file contents\r\n");
    print("  mfetch     - Show system info\r\n");
    print("  del <file> - Delete a file\r\n");
    print("  ren <a> <b>- Rename a file\r\n");
    print("  copy <a> <b>- Copy a file\r\n");
    print("  halt       - Halt the system\r\n");
    print("  create <file> <context> - Creates a file with the given context\r\n");
    print("  time       - Show the current time & date\r\n");
}

static void cmd_cls()              { Reset(); }
static void cmd_ver()              { print("MasterOS v0.6.5 Aero\r\n"); }

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

static void cmd_mfetch() {
    print("\r\n");
    print("   __  __           _             ___  ____ \r\n");
    print("  |  \\/  | __ _ ___| |_ ___ _ __ / _ \\/ ___| \r\n");
    print("  | |\\/| |/ _` / __| __/ _ \\ '__| | | \\___ \\ \r\n");
    print("  | |  | | (_| \\__ \\ ||  __/ |  | |_| |___) |\r\n");
    print("  |_|  |_|\\__,_|___/\\__\\___|_|   \\___/|____/ \r\n");
    print("\r\n");
    print("  OS:      MasterOS v0.6.5 \"Aero\"\r\n");
    print("  Kernel:  Custom x86 (32-bit)\r\n");
    print("  Shell:   mShell\r\n");
    print("  Memory:  ");
    print_uint(total_mem_kb);
    print(" KB\r\n");

    uint32_t total_kb = fat_get_total_kb();
    uint32_t used_kb   = fat_get_used_kb();
    uint32_t free_kb    = (total_kb > used_kb) ? (total_kb - used_kb) : 0;

    Fat12Entry entries[FAT_MAX_FILES];
    int file_count = fat_list(entries, FAT_MAX_FILES);
    if (file_count < 0) file_count = 0;

    print("  Disk:    FAT12/16, ");
    print_uint(used_kb);
    print(" KB used / ");
    print_uint(total_kb);
    print(" KB total (");
    print_uint(free_kb);
    print(" KB free)\r\n");
    print("  Files:   ");
    print_uint(file_count);
    print(" file(s) in root\r\n");
    print("\r\n");
}

static void cmd_create(const char* arg) {
	if (!arg) {
		print ("Usage: create <filename> <context>\r\n");
		return;
	}

	char filename[32];
	k_first_word(arg, filename, 32);
	const char* context = k_arg(arg);
	const uint8_t* file_data = (const uint8_t*)context;

	int context_len = k_strlen(context);
	uint32_t context_size = (uint32_t)context_len;

	if (!context) {
		print ("Usage: create <filename> <context>\r\n");
		return;
	}

	fat_write(filename, file_data, context_size);
	
}

static void cmd_del(const char* arg) {
    if (!arg) { print("Usage: del <filename>\r\n"); return; }

    if (fat_delete(arg) == 0) {
        print("Deleted: ");
        print(arg);
        print("\r\n");
    } else {
        print("File not found: ");
        print(arg);
        print("\r\n");
    }
}

static void cmd_ren(const char* arg) {
    if (!arg) { print("Usage: ren <oldname> <newname>\r\n"); return; }

    char old_name[32];
    k_first_word(arg, old_name, 32);
    const char* new_name = k_arg(arg);

    if (!new_name) { print("Usage: ren <oldname> <newname>\r\n"); return; }

    if (fat_rename(old_name, new_name) == 0) {
        print("Renamed ");
        print(old_name);
        print(" to ");
        print(new_name);
        print("\r\n");
    } else {
        print("File not found: ");
        print(old_name);
        print("\r\n");
    }
}

static void cmd_copy(const char* arg) {
    if (!arg) { print("Usage: copy <source> <destination>\r\n"); return; }

    char src_name[32];
    k_first_word(arg, src_name, 32);
    const char* dst_name = k_arg(arg);

    if (!dst_name) { print("Usage: copy <source> <destination>\r\n"); return; }

    int bytes = fat_read(src_name, file_buf, sizeof(file_buf));
    if (bytes < 0) {
        print("Source not found: ");
        print(src_name);
        print("\r\n");
        return;
    }

    if (fat_write(dst_name, file_buf, (uint32_t)bytes) == 0) {
        print("Copied ");
        print(src_name);
        print(" to ");
        print(dst_name);
        print(" (");
        print_uint((uint32_t)bytes);
        print(" bytes)\r\n");
    } else {
        print("Copy failed (disk full or error).\r\n");
    }
}

static void cmd_time() {
	RTCTime t;
	rtc_read(&t);
	t.hours = t.hours + 3;            // ONLY MODIFY THIS SETTING TO MATCH YOUR
	if (t.hours >= 24) t.hours -= 24; // UTC TIMEZONE (MINE IS +3)

	print_uint(t.hours);
	print(":");
	print_uint(t.minutes);
	
	print("  ");
	
	print_uint(t.day);
	print("/");
	print_uint(t.month);
	print("/");
	print_uint(t.year);
	print(" \r\n");
}

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
    else if (k_strcmp(verb, "mfetch") == 0) cmd_mfetch();
    else if (k_strcmp(verb, "del")    == 0) cmd_del(arg);
    else if (k_strcmp(verb, "ren")    == 0) cmd_ren(arg);
    else if (k_strcmp(verb, "copy")   == 0) cmd_copy(arg);
    else if (k_strcmp(verb, "create")  == 0) cmd_create(arg);
    else if (k_strcmp(verb, "time") == 0) cmd_time();
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

// ─── Init ───

void shell_init() {
    shell_len = 0;
    shell_buf[0] = '\0';
}
