

# get first argument
if [ -z "$1" ]; then
    echo "Usage: $0 <command>"
    exit 1
fi

# declare a variable port = 4560+$1
port=$((4560 + $1))
tcpPort=$((7890 + $1))

# kill the process on the port
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

# call the function
kill_process_on_port $port
kill_process_on_port $tcpPort

# if port == 4560
if [ $port -eq 4560 ]; then
    echo "Starting introducer on port $port $tcpPort"
    ./Daemon $port $tcpPort localhost local introducer 
    # echo "Introducer started on port $port"

# echo "Starting daemon on port $port"
# Replace the following line with your actual daemon command
# For example: your_daemon_command --port $port &
else
    echo "Starting daemon on port $port"
    ./Daemon $port $tcpPort localhost local
fi


# write demo if else
# if [ $port -eq 4560 ]; then
#     echo "Starting introducer on port $port"
# else
#     echo "Starting daemon on port $port"
# fi

