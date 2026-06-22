// SPDX-License-Identifier: MulanPSL-2.0

package cl1

object Cl1BuildMode {
  private def configValue(name: String): Option[String] = {
    sys.props.get(name).orElse(sys.env.get(name)).map(_.trim).filter(_.nonEmpty)
  }

  private def boolValue(name: String, default: Boolean): Boolean = {
    configValue(name) match {
      case Some(value) =>
        value.toLowerCase match {
          case "1" | "true" | "yes" | "y" | "on"  => true
          case "0" | "false" | "no" | "n" | "off" => false
          case other => throw new IllegalArgumentException(s"$name must be boolean, got '$other'")
        }
      case None => default
    }
  }

  private def normalizePlatform(value: String): String = {
    value.toLowerCase.replace("-", "_") match {
      case "simple" | "simple_soc" => "simple_soc"
      case "full" | "full_soc" => "full_soc"
      case other => throw new IllegalArgumentException(s"CL1_PLATFORM must be simple_soc or full_soc, got '$other'")
    }
  }

  val TEST_MODE: String = configValue("cl1.testMode")
    .orElse(configValue("CL1_TEST_MODE"))
    .getOrElse("bus")
    .toLowerCase

  require(
    TEST_MODE == "bus" || TEST_MODE == "cache",
    s"CL1_TEST_MODE must be 'bus' or 'cache', got '$TEST_MODE'"
  )

  val CACHE_MODE: Boolean = TEST_MODE == "cache"

  private val legacyFullSoc = boolValue("CL1_FULL_SOC_TEST", boolValue("CL1_GLOBAL_FULL_SOC_TEST", false))
  private val legacySimpleSoc = boolValue("CL1_SIMPLE_SOC_TEST", boolValue("CL1_GLOBAL_SIMPLE_SOC_TEST", true))
  val PLATFORM: String = configValue("CL1_PLATFORM")
    .orElse(configValue("CL1_ADDRESS_PROFILE"))
    .map(normalizePlatform)
    .getOrElse(if (legacyFullSoc || !legacySimpleSoc) "full_soc" else "simple_soc")

  def bool(name: String, default: Boolean): Boolean = boolValue(name, default)

  def int(name: String, default: Int): Int =
    configValue(name).map(_.toInt).getOrElse(default)

  def string(name: String, default: String): String =
    configValue(name).getOrElse(default)
}

object Cl1Technology {
  val CX55 = "CX55"
  val SMIC55 = "SMIC55"
  val SMIC100 = "SMIC100"

  private val supported = Seq(CX55, SMIC55, SMIC100)

  def normalize(value: String): String = {
    val normalized = value.trim.toUpperCase.replace("-", "_")
    normalized match {
      case CX55 | SMIC55 | SMIC100 => normalized
      case _ =>
        throw new IllegalArgumentException(
          s"CL1_TECHNOLOGY must be one of ${supported.mkString(", ")}, got '$value'"
        )
    }
  }

  def useSmic100Memory(value: String): Boolean = normalize(value) == SMIC100
}

object Cl1BuildProfile {
  private val selectedSimpleSoc = Cl1BuildMode.PLATFORM == "simple_soc"
  private val selectedFullSoc = Cl1BuildMode.PLATFORM == "full_soc"
  val simpleSocTest = Cl1BuildMode.bool("CL1_GLOBAL_SIMPLE_SOC_TEST", selectedSimpleSoc)
  val fullSocTest  = Cl1BuildMode.bool("CL1_GLOBAL_FULL_SOC_TEST", selectedFullSoc)
}

// Synthesis configuration: synthesis flow mode and foundry SRAM macro choices.
object Cl1SynthesisConfig {
  val syn = Cl1BuildMode.bool("CL1_GLOBAL_SYN", Cl1BuildMode.bool("CL1_SYN", !Cl1BuildMode.CACHE_MODE))
  val SramFoundary = syn
  val Technology = Cl1Technology.normalize(Cl1BuildMode.string("CL1_TECHNOLOGY", Cl1Technology.CX55))
}

// Processor configuration: architectural constants, SoC-facing shape,
// reset policy, memory implementation and core micro-architecture knobs.
object Cl1ProcessorConfig {
  private val platform = PlatformAddressMaps.selected
  val BOOT_ADDR  = platform.bootAddrLiteral
  val TVEC_ADDR  = platform.trapVectorLiteral
  val BUS_WIDTH  = 32
  val DBG_ENTRYADDR = "h800"
  val DBG_EXCP_BASE = "h800"
  val MDU_SHAERALU = false
  val WB_PIPESTAGE = true
  val HAS_ICACHE   = Cl1BuildMode.bool("CL1_HAS_ICACHE", Cl1BuildMode.CACHE_MODE)
  val HAS_DCACHE   = Cl1BuildMode.bool("CL1_HAS_DCACHE", Cl1BuildMode.CACHE_MODE)
  val RST_ACTIVELOW = true
  val RST_ASYNC     = true
  val EXPOSE_CORE_BUS = Cl1BuildMode.bool("CL1_EXPOSE_CORE_BUS", !Cl1BuildMode.CACHE_MODE)
  val SOC_D64      = if(Cl1BuildProfile.fullSocTest) true else false
  val CACHE_IDXW = Cl1BuildMode.int("CL1_CACHE_IDXW", Cl1BuildMode.int("CL1_FORMAL_CACHE_IDXW", 7))

  require(
    !(EXPOSE_CORE_BUS && (HAS_ICACHE || HAS_DCACHE)),
    "cache instances are unreachable when EXPOSE_CORE_BUS=true; use CL1_TEST_MODE=cache or disable caches"
  )
}

// Verification configuration: RVFI/formal/difftest and verification-only sizing.
object Cl1VerificationConfig {
  val SOC_DIFF     = Cl1BuildMode.bool("CL1_SOC_DIFF", Cl1BuildProfile.fullSocTest)
  val DIFFTEST     = if(Cl1BuildProfile.simpleSocTest) false else false
  val difftest     = DIFFTEST
  val FORMAL_VERIF = Cl1BuildMode.bool("CL1_FORMAL_VERIF", false)
  val RISCV_FORMAL_ALTOPS = Cl1BuildMode.bool("CL1_RISCV_FORMAL_ALTOPS", false)
  val CACHE_FORMAL = Cl1BuildMode.bool("CL1_CACHE_FORMAL", false)
}

// Low-power configuration: clock gates and reset-saving options.
object Cl1PowerSaveConfig {
  val MODPOWERCFG = false
  val CKG_EN     = false
  val MDU_CKG_EN  = if (MODPOWERCFG) true else false
  val DCACHE_CKG_EN = if (MODPOWERCFG) true else false
  val LSU_CKG_EN    = if (MODPOWERCFG) true else false
  val RF_NORESET    = true
}

// Compatibility facade for existing imports. New code should prefer the
// classified config objects above.
object Cl1Config {
  val BOOT_ADDR = Cl1ProcessorConfig.BOOT_ADDR
  val TVEC_ADDR = Cl1ProcessorConfig.TVEC_ADDR
  val BUS_WIDTH = Cl1ProcessorConfig.BUS_WIDTH
  val CKG_EN = Cl1PowerSaveConfig.CKG_EN
  val difftest = Cl1VerificationConfig.difftest
  val DIFFTEST = Cl1VerificationConfig.DIFFTEST
  val DBG_ENTRYADDR = Cl1ProcessorConfig.DBG_ENTRYADDR
  val DBG_EXCP_BASE = Cl1ProcessorConfig.DBG_EXCP_BASE
  val MDU_SHAERALU = Cl1ProcessorConfig.MDU_SHAERALU
  val WB_PIPESTAGE = Cl1ProcessorConfig.WB_PIPESTAGE
  val HAS_ICACHE = Cl1ProcessorConfig.HAS_ICACHE
  val HAS_DCACHE = Cl1ProcessorConfig.HAS_DCACHE
  val RST_ACTIVELOW = Cl1ProcessorConfig.RST_ACTIVELOW
  val RST_ASYNC = Cl1ProcessorConfig.RST_ASYNC
  val SOC_DIFF = Cl1VerificationConfig.SOC_DIFF
  val SramFoundary = Cl1SynthesisConfig.SramFoundary
  val SOC_D64 = Cl1ProcessorConfig.SOC_D64
  val Technology = Cl1SynthesisConfig.Technology
  val FORMAL_VERIF = Cl1VerificationConfig.FORMAL_VERIF
  val RISCV_FORMAL_ALTOPS = Cl1VerificationConfig.RISCV_FORMAL_ALTOPS
  val CACHE_FORMAL = Cl1VerificationConfig.CACHE_FORMAL
  val EXPOSE_CORE_BUS = Cl1ProcessorConfig.EXPOSE_CORE_BUS
  val CACHE_IDXW = Cl1ProcessorConfig.CACHE_IDXW
}
