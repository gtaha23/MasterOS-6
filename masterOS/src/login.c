#include "login.h"
#include "drivers/vga/vga.h"
#include "drivers/device/keyboard.h"

#define MAX_INPUT 32
#define MAX_ATTEMPTS 3

static const char* VALID_USER = "gtaha";
static const char* VALID_PASS = "135";

static char input_buf[MAX_INPUT];
static int  input_len = 0;
static volatile int  input_done = 0;  // set to 1 when Enter is pressed

static void login_handle_char(char c) {
    if (c == '\n') {
        input_buf[input_len] = '\0';
        input_done = 1;
    } else if (c == '\b') {
        if (input_len > 0) { input_len--; print("\b \b"); }
    } else {
        if (input_len < MAX_INPUT - 1) {
            input_buf[input_len++] = c;
            char tmp[2] = { c, '\0' };
            print(tmp);
        }
    }
}

static void login_handle_pass(char c) {
    if (c == '\n') {
        input_buf[input_len] = '\0';
        input_done = 1;
    } else if (c == '\b') {
        if (input_len > 0) { input_len--; print("\b \b"); }
    } else {
        if (input_len < MAX_INPUT - 1) {
            input_buf[input_len++] = c;
            print("*");
        }
    }
}

static void read_input() {
    input_len = 0;
    input_done = 0;
    keyboard_set_char_handler(login_handle_char);
    while (!input_done) {}
    keyboard_set_char_handler(0);  // reset handler
}

static void read_password() {
    input_len = 0;
    input_done = 0;
    keyboard_set_char_handler(login_handle_pass);
    while (!input_done) {}
    keyboard_set_char_handler(0);
}

static int k_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

void login() {
    print("Welcome to MasterOS!\r\n");
    print("<------------------>\r\n");
    int attempts = 0;
    while (attempts < MAX_ATTEMPTS) {
        print("Username: ");
        read_input();
        if (k_strcmp(input_buf, VALID_USER) != 0) {
            print("\r\nInvalid username.\r\n");
            attempts++;
            continue;
        }
        print("\r\nPassword: ");
        read_password();
        if (k_strcmp(input_buf, VALID_PASS) != 0) {
            print("\r\nInvalid password.\r\n");
            attempts++;
            continue;
        }
        print("\r\nLogin successful!\r\n");
        return;
    }
    print("\r\nToo many failed attempts. System locked.\r\n");
    for(;;); 
}