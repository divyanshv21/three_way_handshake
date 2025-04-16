# Use an official Ubuntu image
FROM ubuntu:20.04

# Prevent interactive prompts from apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Update and install build essentials (g++ and other tools)
RUN apt-get update && apt-get install -y \
    build-essential \
    iproute2 \
    iputils-ping \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Copy the source files to the container
COPY server.cpp /root/server.cpp
COPY client.cpp /root/client.cpp

# Set working directory
WORKDIR /root

# Compile the server and client programs
RUN g++ -o server server.cpp && g++ -o client client.cpp

# Open an interactive shell by default
CMD ["/bin/bash"]
