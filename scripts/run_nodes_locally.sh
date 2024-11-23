
#!/bin/bash

# Define the range of ports
START_PORT=4560
START_TCP_PORT=7890

# Function to start the daemon on a given port
start_daemon() {
    local port=$1
    local tcpPort=$2
    echo "Starting daemon on port $port"
    # Replace the following line with your actual daemon command
    # For example: your_daemon_command --port $port &
    
    nohup ./Daemon $port $tcpPort localhost > /dev/null 2>&1 &
}


echo "Starting introducer on port $START_PORT $START_TCP_PORT"
nohup ./Daemon $START_PORT $START_TCP_PORT localhost introducer > /dev/null 2>&1 &
echo "Introducer started on port $START_PORT"
# Loop through the port range and start daemons
for i in {1..4}
do
    port=$((START_PORT + i))
    tcpPort=$((START_TCP_PORT + i))
    start_daemon $port $tcpPort
done

echo "Daemons started on ports $START_PORT to $END_PORT"