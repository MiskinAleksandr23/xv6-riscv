#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int is_leap(int year) {
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    return (year % 4 == 0);
}

uint days_not_leap[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
uint start_ = 1970;
uint year2days = 365;
uint day2hours = 24;
uint hour2min = 60;
uint min2sec = 60;
uint ms2ns = 1000000;
uint sec2ns = 1000000000;

uint32 get_year(uint32 *days) {
    uint32 yr = start_;
    while (*days >= year2days + is_leap(yr)) {
        *days -= year2days + is_leap(yr);
        yr++;
    }
    return yr;
}

uint32 get_month(uint32 *days, uint32 year) {
    uint32 mth = 0;
    uint32 feb29 = 0;
    
    while (1) {
        if (mth == 1 && is_leap(year)) 
            feb29 = 1;
        if (*days >= days_not_leap[mth] + feb29) {
            *days -= days_not_leap[mth] + feb29;
            mth++;
        }
        else break;
        feb29 = 0;
    }
    return mth;
}

void get_time_parts(uint64 time, uint32 *nsecs, uint32 *msecs, uint64 *secs, 
                   uint32 *mins, uint32 *hrs, uint32 *days) {
    *nsecs = time % sec2ns;
    *msecs = *nsecs / ms2ns;
    *secs = time / sec2ns % min2sec;
    *mins = time / min2sec / sec2ns % hour2min;
    *hrs = time / hour2min / min2sec / sec2ns % day2hours;
    *days = time / day2hours / hour2min / min2sec / sec2ns;
}

void print_two_digits(uint32 num) {
    if (num < 10)
        printf("0%d", num);
    else
        printf("%d", num);
}

void print_msecs(uint32 msecs) {
    char msecs_str[4];
    msecs_str[3] = '\0';
    int i = 2;
    while (i >= 0) {
        msecs_str[i] = '0' + msecs % 10;
        msecs /= 10;
        i--;
    }
    printf("%s", msecs_str);
}

int main(int argc, char* argv[]) {
    uint64 time = rtc();
    
    uint32 nsecs, msecs, mins, hrs, days;
    uint64 secs;
    
    get_time_parts(time, &nsecs, &msecs, &secs, &mins, &hrs, &days);
    
    uint32 yr = get_year(&days);
    uint32 mth = get_month(&days, yr);
    
    printf("Date: ");
    print_two_digits(days + 1);
    printf(".");
    print_two_digits(mth + 1);
    printf(".%d\n", yr);
    
    printf("Time: ");
    print_two_digits(hrs);
    printf(":");
    print_two_digits(mins);
    printf(":");
    print_two_digits(secs);
    printf(".");
    print_msecs(msecs);
    printf("\n");
    
    exit(0);
}