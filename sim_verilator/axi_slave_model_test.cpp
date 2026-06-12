#include "axi_slave_model.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using cl1sim::AxiBurst;
using cl1sim::AxiResp;
using cl1sim::AxiSlaveCycle;
using cl1sim::AxiSlaveInputs;
using cl1sim::AxiSlaveModel;
using cl1sim::BusRequest;
using cl1sim::PendingResponse;

struct MemorySpy {
  std::vector<BusRequest> requests;
  std::vector<bool> fetch_flags;
  std::vector<PendingResponse> responses;
  std::size_t response_index = 0;

  PendingResponse handle(const BusRequest& request, bool is_fetch) {
    requests.push_back(request);
    fetch_flags.push_back(is_fetch);
    if (response_index < responses.size()) {
      return responses[response_index++];
    }
    PendingResponse response;
    response.valid = true;
    response.data = 0;
    response.err = false;
    return response;
  }
};

AxiSlaveCycle step(AxiSlaveModel& model, const AxiSlaveInputs& inputs, MemorySpy& memory) {
  const AxiSlaveCycle cycle = model.sample(inputs);
  model.commit(cycle, [&](const BusRequest& request, bool is_fetch) {
    return memory.handle(request, is_fetch);
  });
  return cycle;
}

AxiSlaveInputs aw(uint32_t addr, uint8_t id, uint8_t len = 0, uint8_t size = 2,
                  AxiBurst burst = AxiBurst::kIncr) {
  AxiSlaveInputs inputs;
  inputs.aw_valid = true;
  inputs.aw_addr = addr;
  inputs.aw_id = id;
  inputs.aw_len = len;
  inputs.aw_size = size;
  inputs.aw_burst = static_cast<uint8_t>(burst);
  return inputs;
}

AxiSlaveInputs w(uint32_t data, uint8_t strb, bool last) {
  AxiSlaveInputs inputs;
  inputs.w_valid = true;
  inputs.w_data = data;
  inputs.w_strb = strb;
  inputs.w_last = last;
  return inputs;
}

AxiSlaveInputs ar(uint32_t addr, uint8_t id, uint8_t len = 0, uint8_t size = 2,
                  AxiBurst burst = AxiBurst::kIncr, bool instr = false) {
  AxiSlaveInputs inputs;
  inputs.ar_valid = true;
  inputs.ar_addr = addr;
  inputs.ar_id = id;
  inputs.ar_len = len;
  inputs.ar_size = size;
  inputs.ar_burst = static_cast<uint8_t>(burst);
  inputs.ar_prot = instr ? 0x4 : 0x0;
  return inputs;
}

void accept_b(AxiSlaveModel& model, MemorySpy& memory) {
  AxiSlaveInputs inputs;
  inputs.b_ready = true;
  const AxiSlaveCycle cycle = step(model, inputs, memory);
  assert(cycle.b_fire);
  assert(!model.outputs().b_valid);
}

void test_w_before_aw_is_not_accepted() {
  AxiSlaveModel model;
  MemorySpy memory;

  AxiSlaveCycle cycle = step(model, w(0x11223344u, 0xf, true), memory);
  assert(!cycle.outputs.w_ready);
  assert(!cycle.w_fire);
  assert(memory.requests.empty());
  assert(!model.outputs().b_valid);

  AxiSlaveInputs both = aw(0x80000000u, 3);
  both.w_valid = true;
  both.w_data = 0x55667788u;
  both.w_strb = 0xf;
  both.w_last = true;
  cycle = step(model, both, memory);
  assert(cycle.aw_fire);
  assert(!cycle.w_fire);
  assert(memory.requests.empty());
  assert(model.outputs().w_ready);

  cycle = step(model, w(0x55667788u, 0xf, true), memory);
  assert(cycle.w_fire);
  assert(memory.requests.size() == 1);
  assert(memory.requests[0].addr == 0x80000000u);
  assert(memory.requests[0].data == 0x55667788u);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 3);
  assert(model.outputs().b_resp == static_cast<uint8_t>(AxiResp::kOkay));

  accept_b(model, memory);
}

void test_write_burst_and_b_backpressure() {
  AxiSlaveModel model;
  MemorySpy memory;

  AxiSlaveCycle cycle = step(model, aw(0x80000010u, 5, 1), memory);
  assert(cycle.aw_fire);

  cycle = step(model, w(0xaaaabbbbu, 0xf, false), memory);
  assert(cycle.w_fire);
  assert(!model.outputs().b_valid);

  cycle = step(model, w(0xccccddddu, 0xf, true), memory);
  assert(cycle.w_fire);
  assert(memory.requests.size() == 2);
  assert(memory.requests[0].addr == 0x80000010u);
  assert(memory.requests[1].addr == 0x80000014u);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 5);

  AxiSlaveInputs blocked_aw = aw(0x80000020u, 6);
  blocked_aw.b_ready = false;
  cycle = step(model, blocked_aw, memory);
  assert(model.outputs().b_valid);
  assert(!cycle.outputs.aw_ready);
  assert(!cycle.aw_fire);

  accept_b(model, memory);
}

void test_fixed_and_wrap_write_addresses() {
  AxiSlaveModel model;
  MemorySpy memory;

  step(model, aw(0x80000020u, 1, 1, 2, AxiBurst::kFixed), memory);
  step(model, w(0x1u, 0xf, false), memory);
  step(model, w(0x2u, 0xf, true), memory);
  assert(memory.requests.size() == 2);
  assert(memory.requests[0].addr == 0x80000020u);
  assert(memory.requests[1].addr == 0x80000020u);
  accept_b(model, memory);

  memory.requests.clear();
  step(model, aw(0x80000008u, 2, 3, 2, AxiBurst::kWrap), memory);
  step(model, w(0x3u, 0xf, false), memory);
  step(model, w(0x4u, 0xf, false), memory);
  step(model, w(0x5u, 0xf, false), memory);
  step(model, w(0x6u, 0xf, true), memory);
  assert(memory.requests.size() == 4);
  assert(memory.requests[0].addr == 0x80000008u);
  assert(memory.requests[1].addr == 0x8000000cu);
  assert(memory.requests[2].addr == 0x80000000u);
  assert(memory.requests[3].addr == 0x80000004u);
  accept_b(model, memory);
}

void test_read_burst_ids_backpressure_and_slverr() {
  AxiSlaveModel model;
  MemorySpy memory;
  memory.responses.push_back(PendingResponse{true, 0x11111111u, false});
  memory.responses.push_back(PendingResponse{true, 0x22222222u, true});

  AxiSlaveCycle cycle = step(model, ar(0x80000000u, 7, 1, 2, AxiBurst::kIncr, true), memory);
  assert(cycle.ar_fire);
  assert(memory.requests.size() == 1);
  assert(memory.requests[0].addr == 0x80000000u);
  assert(memory.fetch_flags[0]);

  assert(model.outputs().r_valid);
  assert(model.outputs().r_id == 7);
  assert(model.outputs().r_data == 0x11111111u);
  assert(model.outputs().r_resp == static_cast<uint8_t>(AxiResp::kOkay));
  assert(!model.outputs().r_last);

  AxiSlaveInputs stall;
  stall.r_ready = false;
  cycle = step(model, stall, memory);
  assert(cycle.outputs.r_valid);
  assert(!cycle.r_fire);
  assert(model.outputs().r_data == 0x11111111u);
  assert(memory.requests.size() == 1);

  AxiSlaveInputs ready;
  ready.r_ready = true;
  cycle = step(model, ready, memory);
  assert(cycle.r_fire);
  assert(memory.requests.size() == 2);
  assert(memory.requests[1].addr == 0x80000004u);
  assert(model.outputs().r_valid);
  assert(model.outputs().r_data == 0x22222222u);
  assert(model.outputs().r_resp == static_cast<uint8_t>(AxiResp::kSlvErr));
  assert(model.outputs().r_resp != static_cast<uint8_t>(AxiResp::kExOkay));
  assert(model.outputs().r_last);

  cycle = step(model, ready, memory);
  assert(cycle.r_fire);
  assert(!model.outputs().r_valid);
}

void test_write_slverr_and_malformed_burst_decerr() {
  AxiSlaveModel model;
  MemorySpy memory;
  memory.responses.push_back(PendingResponse{true, 0, true});

  step(model, aw(0x80000000u, 8), memory);
  step(model, w(0x12345678u, 0xf, true), memory);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 8);
  assert(model.outputs().b_resp == static_cast<uint8_t>(AxiResp::kSlvErr));
  assert(model.outputs().b_resp != static_cast<uint8_t>(AxiResp::kExOkay));
  accept_b(model, memory);

  const std::size_t writes_before_invalid = memory.requests.size();
  step(model, aw(0x80000002u, 9, 3, 2, AxiBurst::kWrap), memory);
  step(model, w(0x1u, 0xf, false), memory);
  step(model, w(0x2u, 0xf, false), memory);
  step(model, w(0x3u, 0xf, false), memory);
  step(model, w(0x4u, 0xf, true), memory);
  assert(memory.requests.size() == writes_before_invalid);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 9);
  assert(model.outputs().b_resp == static_cast<uint8_t>(AxiResp::kDecErr));
}

void test_wlast_mismatch_returns_decerr() {
  AxiSlaveModel model;
  MemorySpy memory;

  step(model, aw(0x80000000u, 10, 1), memory);
  step(model, w(0x11111111u, 0xf, true), memory);
  assert(memory.requests.size() == 1);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 10);
  assert(model.outputs().b_resp == static_cast<uint8_t>(AxiResp::kDecErr));
  accept_b(model, memory);

  step(model, aw(0x80000010u, 11, 0), memory);
  step(model, w(0x22222222u, 0xf, false), memory);
  assert(memory.requests.size() == 2);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 11);
  assert(model.outputs().b_resp == static_cast<uint8_t>(AxiResp::kDecErr));
  accept_b(model, memory);
}

void test_read_and_write_addresses_can_start_together() {
  AxiSlaveModel model;
  MemorySpy memory;
  memory.responses.push_back(PendingResponse{true, 0x12345678u, false});

  AxiSlaveInputs inputs = ar(0x80000000u, 12, 0, 2, AxiBurst::kIncr, true);
  inputs.aw_valid = true;
  inputs.aw_addr = 0x80000020u;
  inputs.aw_id = 13;
  inputs.aw_len = 0;
  inputs.aw_size = 2;
  inputs.aw_burst = static_cast<uint8_t>(AxiBurst::kIncr);

  AxiSlaveCycle cycle = step(model, inputs, memory);
  assert(cycle.ar_fire);
  assert(cycle.aw_fire);
  assert(memory.requests.size() == 1);
  assert(memory.requests[0].addr == 0x80000000u);
  assert(memory.fetch_flags[0]);

  assert(model.outputs().r_valid);
  assert(model.outputs().r_id == 12);
  assert(model.outputs().r_data == 0x12345678u);
  assert(model.outputs().r_last);
  assert(model.outputs().w_ready);

  inputs = w(0xa5a5a5a5u, 0xf, true);
  inputs.r_ready = true;
  cycle = step(model, inputs, memory);
  assert(cycle.r_fire);
  assert(cycle.w_fire);
  assert(memory.requests.size() == 2);
  assert(memory.requests[1].addr == 0x80000020u);
  assert(memory.requests[1].data == 0xa5a5a5a5u);
  assert(!model.outputs().r_valid);
  assert(model.outputs().b_valid);
  assert(model.outputs().b_id == 13);

  accept_b(model, memory);
}

}  // namespace

int main() {
  test_w_before_aw_is_not_accepted();
  test_write_burst_and_b_backpressure();
  test_fixed_and_wrap_write_addresses();
  test_read_burst_ids_backpressure_and_slverr();
  test_write_slverr_and_malformed_burst_decerr();
  test_wlast_mismatch_returns_decerr();
  test_read_and_write_addresses_can_start_together();
  std::cout << "axi_slave_model_test: PASS\n";
  return 0;
}
