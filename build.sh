#!/usr/bin/env bash

set -euox pipefail

# many developers forget to init/update submodules
git submodule update --init --recursive

docker build \
	--target coroutine_asan_failure_packages \
	--tag coroutine_asan_failure_packages \
	./

docker build \
	--target coroutine_asan_failure_build_boost \
	--tag coroutine_asan_failure_build_boost \
	./

docker build \
	--target coroutine_asan_failure_build_example \
	--tag coroutine_asan_failure_build_example \
	./

docker build \
	--target coroutine_asan_failure \
	--tag coroutine_asan_failure \
	./

