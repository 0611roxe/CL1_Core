SHELL := /usr/bin/env bash

SCRIPT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT_DIR := $(abspath $(SCRIPT_DIR)/..)
CL1_TEST_MODE ?= bus
CL1_PLATFORM ?= simple_soc
CL1_SIM := $(SCRIPT_DIR)/cl1_sim.py
TESTS ?= $(SCRIPT_DIR)/../tests
RVDV_ARGS ?=
SIM_ARGS ?=
REGRESSION_SUITE ?= $(if $(SUITE),$(SUITE),full)
LEVEL ?= full
MAX_CYCLES ?=
COMPARE ?=
NO_BUILD_SIM ?=
NO_BUILD_TESTS ?=

.PHONY: build selftest run test regression check riscv-dv clean

build:
	$(CL1_SIM) build --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM)

selftest:
	$(CL1_SIM) selftest --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM)

run:
	@if [ -z "$(IMAGE)" ]; then \
		echo "Usage: make -C sim_verilator run IMAGE=/path/to/test.bin [SIM_ARGS='...']" >&2; \
		exit 1; \
	fi
	cd $(ROOT_DIR) && $(CL1_SIM) run --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM) -- $(SIM_ARGS) $(IMAGE)

test:
	$(CL1_SIM) test --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM) $(TESTS)

regression:
	$(CL1_SIM) regression --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM) --suite $(REGRESSION_SUITE) $(if $(MAX_CYCLES),--max-cycles $(MAX_CYCLES),) $(if $(NO_BUILD_SIM),--no-build-sim,) $(if $(NO_BUILD_TESTS),--no-build-tests,)

check:
	$(CL1_SIM) check --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM) --level $(LEVEL) $(if $(MAX_CYCLES),--max-cycles $(MAX_CYCLES),) $(if $(NO_BUILD_SIM),--no-build-sim,) $(if $(NO_BUILD_TESTS),--no-build-tests,)

riscv-dv:
	$(CL1_SIM) riscv-dv --mode $(CL1_TEST_MODE) --platform $(CL1_PLATFORM) $(if $(COMPARE),--compare,) $(RVDV_ARGS) $(if $(SUITE),$(SUITE),)

clean:
	$(CL1_SIM) clean
