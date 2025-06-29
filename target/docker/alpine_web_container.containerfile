# Build Deployable Image:
#   docker build --tag 'interspec_alpine' -f alpine_web_container.containerfile .

# Run Prebuilt Image:
#   docker run --rm -v path/to/your/datadir:/data -p 8078:8078/tcp interspec_alpine
# Or if you dont care about keeping user preferences and stuff around, you can just just map /data to a temp ephemeral dir
#   docker run --rf -v /data -p 8078:8078/tcp interspec_alpine

# Run From Dockerfile Directly:
#   docker run --rm -v path/to/your/datadir:/data -p 8078:8078 -f alpine_web_container.dockerfile .

#  Optional: reuse existing source, dependency, build directories:
    # To reuse sources, use --build-args hostdepdir=true, place the sources in the build/deps directory and optionally the Interspec sources in ./ .
    # To reuse the complete build directory set --build-args hostbuilddir=true.

# ex: docker build --build-arg hostdepdir=true --build-arg hostbuilddir=true -t interspec -f alpine_web_container.containerfile .
# When hostdepdir=true, the container will reuse everything in ./build/_deps/*-src
# When hostbuilddir=true, the container will reuse everything in the ./build directory and will look for the source repo in the ./ directory.

# If build fails, add the --no-cache option to the docker build command to force a fresh build
#  docker build --no-cache --tag 'interspec_alpine' -f alpine_web_container.dockerfile .

# If you are having trouble with proxies, or certificates, and you feel brave, you can add the "--no-check-certificate" argument to the `apk` 
#  command below to blow past this - but it really isn't a good option.

FROM alpine:3 AS build_interspec
ARG repo=https://github.com/sandialabs/InterSpec.git \
ARG tag=master \
ARG hostdepdir=false \
ARG hostbuilddir=false \
WORKDIR /work

# Optional uncomment and populate directories
# COPY src ./src
# COPY build ./build

RUN --mount=type=bind,source=.,target=/mnt/host,rw    \
        apk add --no-cache \
        alpine-sdk \
        cmake \
        patch \
        linux-headers \
        suitesparse-dev \
        curl \
        uglify-js \
        uglifycss \
        git && \
    if [ "$hostbuilddir" = "true" ] || [ "$hostdepdir" = "true" ]; then \
        echo "Using host build directory" && \
        srcdir=/mnt/host && \
        builddir=/mnt/host/build && \
        if [ "$hostdepdir" = true ] && [ "$hostbuilddir" != true ]; then \
            echo "Using host dependency directory" && \
            mkdir -p /mnt/host/build/_deps && \
            mkdir -p /mnt/host/_deps && \
            mv /mnt/host/build/_deps/*-src /mnt/host/_deps/ 2>/dev/null && \
            rm -rf /mnt/host/build && \
            mkdir -p /mnt/host/build && \
            mv /mnt/host/_deps /mnt/host/build/; \
        fi; \
    else \
        mkdir -p /work/src && \
        mkdir -p /work/build && \
        srcdir=/work/src && \
        builddir=/work/build; \
    fi && \
     echo "Will check for source" && \
    if [ ! -d "$srcdir/InterSpec_resources" ]; then \ 
        echo "source repo missing, cloning" && \
        git clone --recursive --branch $tag --depth=1 $repo $srcdir; \
    fi && \
    echo "About to CMake" && \
    cmake \
        -B "$builddir" \
        -DCONTAINER_MUSL=ON \
        -DInterSpec_FETCH_DEPENDENCIES=ON \
        -S "$srcdir" && \
    echo "About to build" && \
    cmake --build "$builddir" -j4 && \
    echo "About to install" && \
    cmake --install "$builddir" --prefix /release && \
    cd /release && \ 
    chmod -R a+r * && \
    chmod a+x bin/InterSpec && \
    mkdir -p data && \
    chmod 777 data && \
    rm -rf lib && \
    rm -rf include && \
    rm -rf share/eigen3 && \
    rm -rf share/pkgconfig && \
    echo "Build complete."

#Web Server
FROM scratch
LABEL app="InterSpec"
COPY --from=build_interspec /release /interspec/
WORKDIR /interspec
EXPOSE 8078
ENTRYPOINT ["./bin/InterSpec", "--config=./share/interspec/data/config/wt_config_web.xml", "--userdatadir=/data", "--http-port=8078", "--http-address=0.0.0.0", "--docroot", "./share/interspec"]
