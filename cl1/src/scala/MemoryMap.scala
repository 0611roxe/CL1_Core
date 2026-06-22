package cl1

import chisel3._

object MemoryMap {
  private def selected: PlatformAddressMap = PlatformAddressMaps.selected

  def isRegion(addr: UInt, start: UInt, end: UInt): Bool =
    (addr >= start) && (addr <= end)

  def isRAM(addr: UInt): Bool = selected.containsRole(addr, "ram")
  def isDebug(addr: UInt): Bool = selected.containsRole(addr, "debug")
  def isUART(addr: UInt): Bool = selected.containsRole(addr, "uart")
  def isHostExit(addr: UInt): Bool = selected.containsRole(addr, "host_exit")

  def isISRAM(addr: UInt): Bool = selected.containsRegion(addr, "isram")
  def isDSRAM(addr: UInt): Bool = selected.containsRegion(addr, "dsram")
  def isSDRAM(addr: UInt): Bool = selected.containsRegion(addr, "sdram")
  def isQSPIMem(addr: UInt): Bool = selected.containsRegion(addr, "qspi_mem")
  def isDMA(addr: UInt): Bool = selected.containsRole(addr, "dma")
  def isSDIO(addr: UInt): Bool = selected.containsRegion(addr, "sdio")
  def isUART2(addr: UInt): Bool = selected.containsRegion(addr, "uart2")
  def isMMIO(addr: UInt): Bool = selected.isMMIO(addr)
  def isICacheable(addr: UInt): Bool = selected.isICacheable(addr)
  def isDCacheable(addr: UInt): Bool = selected.isDCacheable(addr)
}
