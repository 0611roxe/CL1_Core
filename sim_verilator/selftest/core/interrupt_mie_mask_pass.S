.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MIP_MSIP,      0x00000008

main:
  la t0, unexpected_trap
  csrw mtvec, t0
  csrw mie, zero
  li t0, MSTATUS_MIE
  csrw mstatus, t0

  li t2, 256
wait_pending:
  csrr t0, mip
  andi t0, t0, MIP_MSIP
  bnez t0, pending_seen
  addi t2, t2, -1
  bnez t2, wait_pending
  j fail_no_pending

pending_seen:
  li t2, 64
masked_wait:
  addi t2, t2, -1
  bnez t2, masked_wait

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

.align 2
unexpected_trap:
  li t1, 0x72
  j fail

fail_no_pending:
  li t1, 0x73

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
