FROM ubuntu:22.04 AS coroutine_asan_failure_packages

RUN true \
	&& apt-get update \
	&& DEBIAN_FRONTEND=noninteractive \
		apt-get install -y \
		build-essential \
		cmake \
		curl \
		git \
		libssl-dev \
		gcc-12 \
		g++-12 \
		bzip2 \
		libbz2-dev \
	&& update-alternatives \
		--install /usr/bin/gcc gcc "$(command -v gcc-12)" \
			120 \
		--slave /usr/bin/g++ g++ "$(command -v g++-12)" \
		--slave /usr/bin/gcov gcov "$(command -v gcov-12)" \
	&& rm -Rf /var/lib/apt/lists/* \
	&& true

FROM coroutine_asan_failure_packages AS coroutine_asan_failure_build_boost
WORKDIR /opt/boost/1.81.0
RUN true \
	&& curl -fLOsS 'https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.bz2' \
	&& tar -xf 'boost_1_81_0.tar.bz2' \
	&& cd boost_1_81_0 \
	&& ./bootstrap.sh \
		--prefix=/opt/boost/1.81.0/ \
	&& ./b2 \
		cxxflags="-fPIC -std=c++20" \
		threading=multi \
		-j"$(nproc)" \
	&& ./b2 install \
		--prefix=/opt/boost/1.81.0 \
	&& rm -Rf \
		/opt/boost/1.81.0/boost_1_81_0.tar.bz2 \
		/opt/boost/1.81.0/boost_1_81_0/ \
	&& true

ENV BOOST_ROOT=/opt/boost/1.81.0

FROM coroutine_asan_failure_build_boost AS coroutine_asan_failure_build_example
WORKDIR /usr/src/test/src
COPY ./ ./
WORKDIR /usr/src/test/build

RUN true \
	&& cmake \
		-DCMAKE_BUILD_TYPE='Debug' \
		-DASAN_BUILD=ON \
		/usr/src/test/src \
	&& make -j"$(nproc)" \
	&& true

FROM coroutine_asan_failure_build_example AS coroutine_asan_failure

#
# Default shell is /bin/sh, but bash is needed for process substitution.
# Process substitution is used to capture stdout and stderr to separate files
# while also tee-ing them to the docker build agent.
SHELL [ "/bin/bash", "-c" ]
RUN true \
	&& echo "TESTING WORKAROUND" \
	&& { sleep 1 && curl -sS 'http://127.0.0.1:8080' & } \
	&& ASAN_OPTIONS=detect_stack_use_after_return=true \
		./example \
			1> >(tee ./workaround.txt) \
			2> >(tee ./workaround.err >&2) \
	&& if [ -s ./workaround.err ]; then \
		>&2 echo "Workaround was successful but emitted a message to stderr" \
	;fi \
	&& true

RUN true \
	&& echo "TESTING PLAIN" \
	&& { sleep 1 && curl -sS 'http://127.0.0.1:8080' & } \
	&& ASAN_OPTIONS= \
		./example \
			1> >(tee ./bad.txt) \
			2> >(tee ./bad.err >&2) \
	&& true

CMD ./example
