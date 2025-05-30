FROM --platform=linux/amd64 ubuntu:22.04

# Set working directory
WORKDIR /app

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    mingw-w64 \
    git \
    make \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for cross-compilation
ENV PATH="/usr/bin:${PATH}"
ENV CC="x86_64-w64-mingw32-gcc"
ENV CXX="x86_64-w64-mingw32-g++"
ENV AR="x86_64-w64-mingw32-ar"
ENV RANLIB="x86_64-w64-mingw32-ranlib"
ENV WINDRES="x86_64-w64-mingw32-windres"

# Create Windows headers directory
RUN mkdir -p /usr/x86_64-w64-mingw32/include/windows

# Copy the source code
COPY . .

# Build command
CMD ["make", "-C", "PowerEditor/gcc"] 
