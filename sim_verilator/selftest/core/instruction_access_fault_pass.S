.section .text
.globl main

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ NOEXEC_ADDR,    0x60000000

main:
  la t0, trap_handler
  csrw mtvec, t0

  li s0, 0
  la s1, fetch_fault_target
  la s2, fetch_fault_after
  li s3, NOEXEC_ADDR

  jr s3

fetch_fault_after:
  li t0, 1
  bne s0, t0, fail

  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)

pass_loop:
  j pass_loop

.align 2
trap_handler:
  csrr t0, mcause
  li t1, 1
  bne t0, t1, fail

  csrr t0, mepc
  bne t0, s3, fail

  csrr t0, mtval
  beqz t0, trap_recover
  bne t0, s3, fail

trap_recover:
  addi s0, s0, 1
  csrw mepc, s2
  mret

fail:
  li t0, HOST_EXIT_ADDR
  li t1, 0x41
  sw t1, 0(t0)

fail_loop:
  j fail_loop

.section .fault_region_data, "aw", @progbits
.balign 4
fetch_fault_target:
  .word 0x00000013
