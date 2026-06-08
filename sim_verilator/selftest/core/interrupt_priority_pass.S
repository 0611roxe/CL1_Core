.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MIE_MSIE,      0x00000008
.equ MIE_MTIE,      0x00000080
.equ MIE_MEIE,      0x00000800
.equ MIP_MSIP,      0x00000008
.equ MIP_MTIP,      0x00000080
.equ MIP_MEIP,      0x00000800
.equ MCAUSE_MEI,    0x8000000b

main:
  la t0, interrupt_handler
  csrw mtvec, t0
  csrw mie, zero
  csrw mstatus, zero

  li t2, 512
  li t3, MIP_MEIP | MIP_MSIP | MIP_MTIP
wait_all_pending:
  csrr t0, mip
  and t0, t0, t3
  beq t0, t3, enable_all
  addi t2, t2, -1
  bnez t2, wait_all_pending
  li t1, 0x7e
  j fail

enable_all:
  li t0, MIE_MEIE | MIE_MSIE | MIE_MTIE
  csrw mie, t0
  li t0, MSTATUS_MIE
  csrw mstatus, t0

wait_irq:
  addi s0, s0, 1
  j wait_irq

.align 2
interrupt_handler:
  csrr t0, mcause
  li t1, MCAUSE_MEI
  bne t0, t1, fail_bad_cause

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

fail_bad_cause:
  li t1, 0x7d

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
