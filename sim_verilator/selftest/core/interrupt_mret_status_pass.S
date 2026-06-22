.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MSTATUS_MPIE,  0x00000080
.equ MIE_MSIE,      0x00000008
.equ MCAUSE_MSI,    0x80000003
.equ HANDLER_MARK,  0x13572468

main:
  la t0, interrupt_handler
  csrw mtvec, t0
  li t0, MIE_MSIE
  csrw mie, t0
  li t0, MSTATUS_MIE
  csrw mstatus, t0

wait_irq:
  addi s0, s0, 1
  j wait_irq

after_mret:
  li t0, HANDLER_MARK
  bne s1, t0, fail_marker

  csrr t0, mstatus
  andi t1, t0, MSTATUS_MIE
  beqz t1, fail_mret_mie
  andi t1, t0, MSTATUS_MPIE
  beqz t1, fail_mret_mpie

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

.align 2
interrupt_handler:
  csrr t0, mcause
  li t1, MCAUSE_MSI
  bne t0, t1, fail_bad_cause

  csrr t0, mstatus
  andi t1, t0, MSTATUS_MIE
  bnez t1, fail_trap_mie
  andi t1, t0, MSTATUS_MPIE
  beqz t1, fail_trap_mpie

  csrw mie, zero
  li s1, HANDLER_MARK
  la t0, after_mret
  csrw mepc, t0
  mret

fail_bad_cause:
  li t1, 0x77
  j fail
fail_trap_mie:
  li t1, 0x78
  j fail
fail_trap_mpie:
  li t1, 0x79
  j fail
fail_marker:
  li t1, 0x7a
  j fail
fail_mret_mie:
  li t1, 0x7b
  j fail
fail_mret_mpie:
  li t1, 0x7c

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
