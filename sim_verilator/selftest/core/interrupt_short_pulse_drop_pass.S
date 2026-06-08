.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MIE_MSIE,      0x00000008

main:
  la t0, unexpected_trap
  csrw mtvec, t0
  li t0, MIE_MSIE
  csrw mie, t0
  li t0, MSTATUS_MIE
  csrw mstatus, t0

  li t2, 512
wait_pulses:
  addi t2, t2, -1
  bnez t2, wait_pulses

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

.align 2
unexpected_trap:
  li t1, 0x80

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
