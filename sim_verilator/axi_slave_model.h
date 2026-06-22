#ifndef SIM_VERILATOR_AXI_SLAVE_MODEL_H_
#define SIM_VERILATOR_AXI_SLAVE_MODEL_H_

#include <cstdint>
#include <functional>

#include "common.h"

namespace cl1sim {

enum class AxiResp : uint8_t {
  kOkay = 0,
  kExOkay = 1,
  kSlvErr = 2,
  kDecErr = 3
};

enum class AxiBurst : uint8_t {
  kFixed = 0,
  kIncr = 1,
  kWrap = 2
};

struct AxiSlaveInputs {
  bool ar_valid = false;
  uint32_t ar_addr = 0;
  uint8_t ar_id = 0;
  uint8_t ar_len = 0;
  uint8_t ar_size = 2;
  uint8_t ar_burst = static_cast<uint8_t>(AxiBurst::kIncr);
  uint8_t ar_prot = 0;
  bool r_ready = false;

  bool aw_valid = false;
  uint32_t aw_addr = 0;
  uint8_t aw_id = 0;
  uint8_t aw_len = 0;
  uint8_t aw_size = 2;
  uint8_t aw_burst = static_cast<uint8_t>(AxiBurst::kIncr);

  bool w_valid = false;
  uint32_t w_data = 0;
  uint8_t w_strb = 0;
  bool w_last = false;
  bool b_ready = false;
};

struct AxiSlaveOutputs {
  bool ar_ready = false;
  bool r_valid = false;
  uint32_t r_data = 0;
  uint8_t r_resp = static_cast<uint8_t>(AxiResp::kOkay);
  bool r_last = false;
  uint8_t r_id = 0;

  bool aw_ready = false;
  bool w_ready = false;
  bool b_valid = false;
  uint8_t b_resp = static_cast<uint8_t>(AxiResp::kOkay);
  uint8_t b_id = 0;
};

struct AxiSlaveCycle {
  AxiSlaveInputs inputs;
  AxiSlaveOutputs outputs;
  bool ar_fire = false;
  bool r_fire = false;
  bool aw_fire = false;
  bool w_fire = false;
  bool b_fire = false;
};

class AxiSlaveModel {
 public:
  using MemoryCallback = std::function<PendingResponse(const BusRequest& request, bool is_fetch)>;

  void reset();
  AxiSlaveOutputs outputs() const;
  AxiSlaveCycle sample(const AxiSlaveInputs& inputs) const;
  void commit(const AxiSlaveCycle& cycle, const MemoryCallback& memory);

  static uint32_t beat_addr(uint32_t base, uint8_t beat, uint8_t size, uint8_t burst, uint8_t len);

 private:
  struct ReadContext {
    bool active = false;
    uint32_t addr = 0;
    uint8_t id = 0;
    uint8_t len = 0;
    uint8_t beat = 0;
    uint8_t size = 2;
    uint8_t burst = static_cast<uint8_t>(AxiBurst::kIncr);
    bool instr = false;
    uint8_t param_resp = static_cast<uint8_t>(AxiResp::kOkay);
    uint32_t data = 0;
    uint8_t resp = static_cast<uint8_t>(AxiResp::kOkay);
  };

  struct WriteContext {
    bool active = false;
    uint32_t addr = 0;
    uint8_t id = 0;
    uint8_t len = 0;
    uint8_t beat = 0;
    uint8_t size = 2;
    uint8_t burst = static_cast<uint8_t>(AxiBurst::kIncr);
    uint8_t param_resp = static_cast<uint8_t>(AxiResp::kOkay);
    uint8_t resp = static_cast<uint8_t>(AxiResp::kOkay);
  };

  static uint32_t transfer_bytes(uint8_t size);
  static uint8_t response_from_memory(const PendingResponse& response);
  static uint8_t combine_resp(uint8_t lhs, uint8_t rhs);
  static uint8_t validate_burst(uint32_t addr, uint8_t len, uint8_t size, uint8_t burst);
  void prepare_read_beat(const MemoryCallback& memory);

  ReadContext read_;
  WriteContext write_;
  bool b_active_ = false;
  uint8_t b_id_ = 0;
  uint8_t b_resp_ = static_cast<uint8_t>(AxiResp::kOkay);
};

}  // namespace cl1sim

#endif  // SIM_VERILATOR_AXI_SLAVE_MODEL_H_
