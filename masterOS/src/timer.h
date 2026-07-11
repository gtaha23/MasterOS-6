void initTimer();
void onIRQ0(struct InterruptRegisters *regs);

extern uint32_t timer_ticks;
uint32_t GUS();