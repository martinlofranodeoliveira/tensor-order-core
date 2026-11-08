# SPDX-License-Identifier: MIT
# Copyright 2026 Lofrano Tecnologia
.PHONY: all configure build check sanitize test demo

all: build

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

build: configure
	cmake --build build -j$(nproc)

check: build
	ctest --test-dir build --output-on-failure

sanitize:
	cmake -S . -B build-san -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DTOC_SANITIZE=ON
	cmake --build build-san -j$(nproc)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir build-san --output-on-failure

test: check sanitize

demo: build
	./build/tensor-order-cli demo --assert
	./build/tensor-order-cli demo --json
