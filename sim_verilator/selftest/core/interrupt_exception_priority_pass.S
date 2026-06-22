.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MIE_MSIE,      0x00000008
.equ MIP_MSIP,      0x00000008
.equ MCAUSE_ILLEGAL, 0x00000002
.equ MCAUSE_MSI,    0x80000003

main:
  la t0, trap_handler
  csrw mtvec, t0
  li t0, MIE_MSIE
  csrw mie, t0
  csrw mstatus, zero

  li t2, 256
wait_pending:
  csrr t0, mip
  andi t0, t0, MIP_MSIP
  bnez t0, arm_race
  addi t2, t2, -1
  bnez t2, wait_pending
  li t1, 0x81
  j fail

arm_race:
  li t0, MSTATUS_MIE
  csrw mstatus, t0

illegal_insn:
  .word 0xffffffff

after_illegal:
  li t1, 0x82
  j fail

.align 2
trap_handler:
  csrr t0, mcause
  li t1, MCAUSE_ILLEGAL
  beq t0, t1, pass
  li t1, MCAUSE_MSI
  beq t0, t1, fail_interrupt_won
  li t1, 0x83
  j fail

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

fail_interrupt_won:
  li t1, 0x84

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
