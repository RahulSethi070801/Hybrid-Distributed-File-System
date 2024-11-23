#!/bin/bash

# Check if the number of ports is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <number_of_ports>"
    exit 1
fi

# Number of ports to check (x)
NUM_PORTS=$1

# Starting port
START_PORT=4560
START_TCP_PORT=7890

# Function to kill process on a given port
kill_process_on_port() {
    local port=$1
    local pid=$(lsof -ti :$port)
    if [ ! -z "$pid" ]; then
        echo "Killing process on port $port (PID: $pid)"
        kill -9 $pid
    else
        echo "No process found on port $port"
    fi
}

# Loop through ports and kill processes
for i in $(seq 0 $NUM_PORTS)
do
    port=$((START_PORT + i))
    tcpPort=$((START_TCP_PORT + i))
    kill_process_on_port $port
    kill_process_on_port $tcpPort
done

echo "Finished killing processes on ports $START_PORT to $((START_PORT + NUM_PORTS))"