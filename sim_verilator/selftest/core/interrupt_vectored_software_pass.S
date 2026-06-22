.section .text
.globl main
.option norvc

#include "address_map.h"

.equ HOST_EXIT_ADDR, CL1_DEFAULT_HOST_EXIT_ADDR
.equ MSTATUS_MIE,   0x00000008
.equ MIE_MSIE,      0x00000008
.equ MCAUSE_MSI,    0x80000003

main:
  la t0, vector_base
  ori t0, t0, 1
  csrw mtvec, t0
  li t0, MIE_MSIE
  csrw mie, t0
  li t0, MSTATUS_MIE
  csrw mstatus, t0

wait_irq:
  addi s0, s0, 1
  j wait_irq

.align 6
vector_base:
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j software_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector
  j wrong_vector

software_vector:
  csrr t0, mcause
  li t1, MCAUSE_MSI
  bne t0, t1, fail_bad_cause

pass:
  li t0, HOST_EXIT_ADDR
  sw zero, 0(t0)
pass_loop:
  j pass_loop

wrong_vector:
  li t1, 0x7e
  j fail

fail_bad_cause:
  li t1, 0x7f

fail:
  li t0, HOST_EXIT_ADDR
  sw t1, 0(t0)
fail_loop:
  j fail_loop
