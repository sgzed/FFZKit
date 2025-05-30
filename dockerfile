FROM ubuntu:20.04 AS build
ARG MODEL

RUN apt-get update && \
         DEBIAN_FRONTEND="noninteractive" \
         apt-get install -y --no-install-recommends \
         build-essential \
         cmake \
         git \
         curl \
         vim \
         wget \
         ca-certificates \
         tzdata \
         libssl-dev \
         gcc \
         g++ \
         gdb && \
         apt-get autoremove -y && \
         apt-get clean -y && \
         rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/media
COPY . /opt/media/FFZKit
WORKDIR /opt/media/FFZKit

RUN mkdir -p build release/linux/${MODEL}/

WORKDIR /opt/media/FFZKit/build
RUN cmake -DCMAKE_BUILD_TYPE=${MODEL} .. && \
    make -j $(nproc)


FROM ubuntu:20.04
ARG MODEL

ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
        && echo $TZ > /etc/timezone && \
        mkdir -p /opt/media/bin

WORKDIR /opt/media/bin/
COPY --from=build /opt/media/FFZKit/release/linux/${MODEL}/test_noticeCenter  /opt/media/bin/
COPY --from=build /opt/media/FFZKit/release/linux/${MODEL}/libFFZKit.so  /opt/media/bin/

RUN ls -l /opt/media/bin/

ENV PATH /opt/media/bin:$PATH
ENV LD_LIBRARY_PATH /opt/media/bin:$LD_LIBRARY_PATH
CMD ["./test_noticeCenter"]
