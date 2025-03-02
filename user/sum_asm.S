.global main

main:
    addi sp, sp, -32
    mv a0, sp
    li a1, 32
    call gets
    mv t0, a0
    call strip_newline
    la a0, fmt_input
    mv a1, sp
    call printf
    li t4, 31
    li t6, 0
    mv a0, sp
    call cnt_chars
    bgt t6, t4, overflow_error
    mv a0, sp
    call find_first_space
    addi a0, a0, 1
    call single_space
    li t4, ' '
    mv a0, sp
    lb t3, 0(a0)
    beq t3, t4, format_error
    mv a0, sp
    call cnt_chars
    add a0, sp, t6
    lb t3, -1(a0)
    beq t3, t4, format_error
    call parse_first_number
    addi sp, sp, 32
    li a0, 0
    call exit

strip_newline:
    lb t1, 0(t0)
    beqz t1, sn_done
    li t2, '\n'
    beq t1, t2, sn_replace
    addi t0, t0, 1
    j strip_newline
sn_replace:
    sb zero, 0(t0)
sn_done:
    ret

cnt_chars:
    lb t1, 0(a0)
    beqz t1, cnt_end
    addi t6, t6, 1
    addi a0, a0, 1
    j cnt_chars
cnt_end:
    ret

find_first_space:
    lb t1, 0(a0)
    beqz t1, format_error
    li t2, ' '
    beq t1, t2, fs_found
    addi a0, a0, 1
    j find_first_space
fs_found:
    ret

single_space:
    lb t1, 0(a0)
    beqz t1, vs_done
    li t2, ' '
    beq t1, t2, format_error
    addi a0, a0, 1
    j single_space
vs_done:
    ret

parse_first_number:
    li t4, 0
    li t5, '-'
    mv a0, sp
    lb t1, 0(a0)
    beq t1, t5, first_negative
    call first_number
    ret

first_negative:
    li t4, 1
    addi a0, a0, 1
    call first_number
    sub a0, zero, a0
    ret

first_number:
    call atoi
    bne t4, zero, first_negate
    j first_proceed
first_negate:
    sub a0, zero, a0
first_proceed:
    j parse_second_number

parse_second_number:
    li t4, 0
    li t5, '-'
    mv s0, a0
    mv a0, sp
    call find_first_space
    addi a0, a0, 1
    lb t1, 0(a0)
    beq t1, t5, second_negative
    call second_number
    ret

second_negative:
    li t4, 1
    addi a0, a0, 1
    call second_number
    sub a0, zero, a0
    ret

second_number:
    call atoi
    bne t4, zero, second_negate
    j second_proceed
second_negate:
    sub a0, zero, a0
second_proceed:
    j do_sum

do_sum:
    mv s1, a0
    add a0, s0, s1
    mv a1, a0
    la a0, fmt_sum
    call printf
    li a0, 0
    call exit

format_error:
    la a0, error_many_spaces
    call printf
    li a0, 1
    call exit

overflow_error:
    la a0, error_buffer_overflow
    call printf
    li a0, 1
    call exit

.section .rodata
fmt_input:             .string "|%s|\n"
fmt_sum:               .string "%d\n"
error_many_spaces:     .string "Ошибка: строка не имеет вид |int int|\n"
error_buffer_overflow: .string "Ошибка: переполнение буфера\n"
