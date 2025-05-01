#include "types.h"
#include "memlayout.h"
#include "riscv.h"

uint32 rtc_low(void) {
    return *(volatile uint32*) RTC_LOW;
}
uint32 rtc_high(void) {
    return *(volatile uint32*) RTC_HIGH;
}
uint64 rtc_read(void) {
    uint32 low = rtc_low();
    uint32 high = rtc_high();

    return (((uint64) high) << 32) + low;
}