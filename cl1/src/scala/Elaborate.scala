object Elaborate extends App {
  private def topKind(default: String): String =
    sys.props.get("CL1_ELAB_TOP")
      .orElse(sys.env.get("CL1_ELAB_TOP"))
      .map(_.trim)
      .filter(_.nonEmpty)
      .getOrElse(default)
      .toLowerCase

  private def topName(default: String): String =
    sys.props.get("CL1_TOP_NAME")
      .orElse(sys.env.get("CL1_TOP_NAME"))
      .map(_.trim)
      .filter(_.nonEmpty)
      .getOrElse(default)

  val firtoolOptions = Array(
    "--lowering-options=" + List(
      // make yosys happy
      // see https://github.com/llvm/circt/blob/main/docs/VerilogGeneration.md
      "disallowLocalVariables",
      "disallowPackedArrays",
      "locationInfoStyle=wrapInAtSquareBracket"
    ).reduce(_ + "," + _),
    "--disable-all-randomization",
    // "-o=vsrc/sv-gen",
    // "--split-verilog"
    "--ckg-name=HVT_CLKLANQHDV4",
    "--ckg-test-enable=TE",
    "--ckg-input=CK",
    "--ckg-enable=E",
    "--ckg-output=Q"
  )
  topKind("core") match {
    case "core" =>
      circt.stage.ChiselStage.emitSystemVerilogFile(
        new cl1.Cl1Top {
          override def desiredName: String = topName("Cl1Top")
        },
        args,
        firtoolOptions
      )
    case "cache" =>
      circt.stage.ChiselStage.emitSystemVerilogFile(
        new cl1.Cl1CacheFormal {
          override def desiredName: String = topName("Cl1CacheFormal")
        },
        args,
        firtoolOptions
      )
    case other =>
      throw new IllegalArgumentException(s"CL1_ELAB_TOP must be core or cache, got '$other'")
  }
}
  
