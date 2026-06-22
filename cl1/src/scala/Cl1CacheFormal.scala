// SPDX-License-Identifier: MulanPSL-2.0

package cl1

import chisel3._
import chisel3.util._
import cl1.Cl1Config._

class Cl1CacheFormal extends Module {
  require(CACHE_FORMAL, "Cl1CacheFormal requires CL1_CACHE_FORMAL=true")

  val io = IO(new Bundle {
    val icore = Flipped(new CoreBus)
    val dcore = Flipped(new CoreBus)
    val idxReq = Flipped(Decoupled(new dxReq))
    val ddxReq = Flipped(Decoupled(new dxReq))
    val master = new AXI4(BUS_WIDTH, BUS_WIDTH, 2)
    val icache_idle = Output(Bool())
    val dcache_idle = Output(Bool())
    val dcacheWriteback = if (CACHE_FORMAL) Some(Output(new DCacheWritebackFormalObserve)) else None
  })

  val icache = Module(new Cl1ICACHE)
  val dcache = Module(new Cl1DCACHE)
  val xbar = Module(new crossbarCache)

  io.icore <> icache.io.in
  io.idxReq <> icache.io.dxReq
  io.icache_idle := icache.io.icache_idle
  xbar.io.in(0) <> icache.io.out

  io.dcore <> dcache.io.in
  io.ddxReq <> dcache.io.dxReq
  io.dcache_idle := dcache.io.dcache_idle
  io.dcacheWriteback.foreach(_ := dcache.io.formalWriteback.get)
  xbar.io.in(1) <> dcache.io.out

  io.master <> xbar.io.out
}
