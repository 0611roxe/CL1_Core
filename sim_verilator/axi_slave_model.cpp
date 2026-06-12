#include "axi_slave_model.h"

#include <algorithm>

namespace cl1sim {

void AxiSlaveModel::reset() {
  read_ = ReadContext{};
  write_ = WriteContext{};
  b_active_ = false;
  b_id_ = 0;
  b_resp_ = static_cast<uint8_t>(AxiResp::kOkay);
}

AxiSlaveOutputs AxiSlaveModel::outputs() const {
  AxiSlaveOutputs out;

  out.ar_ready = !read_.active;
  out.r_valid = read_.active;
  out.r_data = read_.data;
  out.r_resp = read_.resp;
  out.r_last = read_.active && read_.beat == read_.len;
  out.r_id = read_.id;

  out.aw_ready = !write_.active && !b_active_;
  out.w_ready = write_.active;
  out.b_valid = b_active_;
  out.b_resp = b_resp_;
  out.b_id = b_id_;

  return out;
}

AxiSlaveCycle AxiSlaveModel::sample(const AxiSlaveInputs& inputs) const {
  AxiSlaveCycle cycle;
  cycle.inputs = inputs;
  cycle.outputs = outputs();
  cycle.ar_fire = inputs.ar_valid && cycle.outputs.ar_ready;
  cycle.r_fire = cycle.outputs.r_valid && inputs.r_ready;
  cycle.aw_fire = inputs.aw_valid && cycle.outputs.aw_ready;
  cycle.w_fire = inputs.w_valid && cycle.outputs.w_ready;
  cycle.b_fire = cycle.outputs.b_valid && inputs.b_ready;
  return cycle;
}

void AxiSlaveModel::commit(const AxiSlaveCycle& cycle, const MemoryCallback& memory) {
  if (cycle.r_fire) {
    if (read_.beat == read_.len) {
      read_ = ReadContext{};
    } else {
      ++read_.beat;
      prepare_read_beat(memory);
    }
  }

  if (cycle.b_fire) {
    b_active_ = false;
    b_id_ = 0;
    b_resp_ = static_cast<uint8_t>(AxiResp::kOkay);
  }

  if (cycle.ar_fire) {
    read_.active = true;
    read_.addr = cycle.inputs.ar_addr;
    read_.id = cycle.inputs.ar_id;
    read_.len = cycle.inputs.ar_len;
    read_.beat = 0;
    read_.size = cycle.inputs.ar_size;
    read_.burst = cycle.inputs.ar_burst;
    read_.instr = (cycle.inputs.ar_prot & 0x4u) != 0;
    read_.param_resp = validate_burst(read_.addr, read_.len, read_.size, read_.burst);
    read_.resp = read_.param_resp;
    read_.data = 0;
    prepare_read_beat(memory);
  }

  if (cycle.aw_fire) {
    write_.active = true;
    write_.addr = cycle.inputs.aw_addr;
    write_.id = cycle.inputs.aw_id;
    write_.len = cycle.inputs.aw_len;
    write_.beat = 0;
    write_.size = cycle.inputs.aw_size;
    write_.burst = cycle.inputs.aw_burst;
    write_.param_resp = validate_burst(write_.addr, write_.len, write_.size, write_.burst);
    write_.resp = write_.param_resp;
  }

  if (cycle.w_fire) {
    uint8_t beat_resp = static_cast<uint8_t>(AxiResp::kOkay);
    const bool expected_last = write_.beat == write_.len;
    if (cycle.inputs.w_last != expected_last) {
      beat_resp = combine_resp(beat_resp, static_cast<uint8_t>(AxiResp::kDecErr));
    }

    if (write_.param_resp == static_cast<uint8_t>(AxiResp::kOkay)) {
      BusRequest request;
      request.addr = beat_addr(write_.addr, write_.beat, write_.size, write_.burst, write_.len);
      request.data = cycle.inputs.w_data;
      request.mask = cycle.inputs.w_strb;
      request.size = write_.size;
      request.wen = true;
      request.instr = false;
      beat_resp = combine_resp(beat_resp, response_from_memory(memory(request, false)));
    } else {
      beat_resp = combine_resp(beat_resp, write_.param_resp);
    }

    write_.resp = combine_resp(write_.resp, beat_resp);

    if (cycle.inputs.w_last || expected_last) {
      b_active_ = true;
      b_id_ = write_.id;
      b_resp_ = write_.resp;
      write_ = WriteContext{};
    } else {
      ++write_.beat;
    }
  }
}

uint32_t AxiSlaveModel::beat_addr(uint32_t base, uint8_t beat, uint8_t size, uint8_t burst, uint8_t len) {
  const uint32_t bytes = transfer_bytes(size);
  if (bytes == 0) {
    return base;
  }

  switch (static_cast<AxiBurst>(burst)) {
    case AxiBurst::kFixed:
      return base;
    case AxiBurst::kIncr:
      return base + static_cast<uint32_t>(beat) * bytes;
    case AxiBurst::kWrap: {
      const uint32_t span = bytes * (static_cast<uint32_t>(len) + 1u);
      if (span == 0) {
        return base;
      }
      const uint32_t boundary = (base / span) * span;
      const uint32_t offset = (base - boundary + static_cast<uint32_t>(beat) * bytes) % span;
      return boundary + offset;
    }
  }

  return base;
}

uint32_t AxiSlaveModel::transfer_bytes(uint8_t size) {
  if (size >= 31) {
    return 0;
  }
  return 1u << size;
}

uint8_t AxiSlaveModel::response_from_memory(const PendingResponse& response) {
  return response.err ? static_cast<uint8_t>(AxiResp::kSlvErr) : static_cast<uint8_t>(AxiResp::kOkay);
}

uint8_t AxiSlaveModel::combine_resp(uint8_t lhs, uint8_t rhs) {
  if (lhs == static_cast<uint8_t>(AxiResp::kDecErr) || rhs == static_cast<uint8_t>(AxiResp::kDecErr)) {
    return static_cast<uint8_t>(AxiResp::kDecErr);
  }
  if (lhs == static_cast<uint8_t>(AxiResp::kSlvErr) || rhs == static_cast<uint8_t>(AxiResp::kSlvErr)) {
    return static_cast<uint8_t>(AxiResp::kSlvErr);
  }
  return static_cast<uint8_t>(AxiResp::kOkay);
}

uint8_t AxiSlaveModel::validate_burst(uint32_t addr, uint8_t len, uint8_t size, uint8_t burst) {
  if (size > 2) {
    return static_cast<uint8_t>(AxiResp::kDecErr);
  }
  if (burst > static_cast<uint8_t>(AxiBurst::kWrap)) {
    return static_cast<uint8_t>(AxiResp::kDecErr);
  }

  const uint32_t bytes = transfer_bytes(size);
  const uint32_t beats = static_cast<uint32_t>(len) + 1u;
  if (bytes == 0) {
    return static_cast<uint8_t>(AxiResp::kDecErr);
  }

  if (burst == static_cast<uint8_t>(AxiBurst::kWrap)) {
    const bool valid_wrap_len = len == 1 || len == 3 || len == 7 || len == 15;
    if (!valid_wrap_len || (addr & (bytes - 1u)) != 0) {
      return static_cast<uint8_t>(AxiResp::kDecErr);
    }
  }

  const uint64_t last_addr = static_cast<uint64_t>(addr) + static_cast<uint64_t>(bytes) * beats - 1u;
  if ((static_cast<uint64_t>(addr) >> 12) != (last_addr >> 12)) {
    return static_cast<uint8_t>(AxiResp::kDecErr);
  }

  return static_cast<uint8_t>(AxiResp::kOkay);
}

void AxiSlaveModel::prepare_read_beat(const MemoryCallback& memory) {
  if (!read_.active) {
    return;
  }

  if (read_.param_resp != static_cast<uint8_t>(AxiResp::kOkay)) {
    read_.data = 0;
    read_.resp = read_.param_resp;
    return;
  }

  BusRequest request;
  request.addr = beat_addr(read_.addr, read_.beat, read_.size, read_.burst, read_.len);
  request.mask = kFullWordMask;
  request.size = read_.size;
  request.wen = false;
  request.instr = read_.instr;

  const PendingResponse response = memory(request, read_.instr);
  read_.data = response.data;
  read_.resp = response_from_memory(response);
}

}  // namespace cl1sim
