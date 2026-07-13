void initKeyb();
void keybHandler(struct InterruptRegisters *regs);
void keyboard_set_char_handler(void (*handler)(char));