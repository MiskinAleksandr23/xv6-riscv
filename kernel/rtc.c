#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock rtc_lock;

void rtcinit() {
    initlock(&rtc_lock, "main");
}

uint32 rtc_low(void) {
    return *(volatile uint32*) RTC_LOW;
}
uint32 rtc_high(void) {
    return *(volatile uint32*) RTC_HIGH;
}
uint64 rtc_read(void) {

    acquire(&rtc_lock);
    uint32 low = rtc_low();
    uint32 high = rtc_high();
    release(&rtc_lock);

    return (((uint64) high) << 32) + low;
}