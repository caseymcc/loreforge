#!/bin/bash

# Script to manage the loreforge gRPC server inside the Docker container

# --- Configuration ---
OS="linux"
ARCH="x64"
BUILD_TYPE="debug"
PID_FILE="/tmp/loreforge.pid"
LOG_FILE="/tmp/loreforge.log"
SERVER_READY_MESSAGE="Server listening on"
START_TIMEOUT=15 # seconds

# --- Helper Functions ---
usage() {
    echo "Usage: $0 [options] [command]"
    echo "Commands:"
    echo "  start    Start the server (default action)."
    echo "  stop     Stop the server."
    echo "Options:"
    echo "  --os <os>          Set the operating system (default: linux)."
    echo "  --arch <arch>      Set the architecture (default: x64)."
    echo "  --build <type>     Set the build type (default: debug)."
    echo "  -h, --help         Display this help message."
}

# Function to stop the server process
stop_server() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null; then
            echo "Stopping server with PID: $PID..."
            kill "$PID"
            sleep 1
            if ps -p "$PID" > /dev/null; then
                echo "Server did not stop gracefully, killing..."
                kill -9 "$PID"
            fi
            echo "Server stopped."
        else
            echo "PID file found, but no process with PID $PID is running. Removing stale PID file."
        fi
        rm -f "$PID_FILE"
    else
        echo "Server is not running."
    fi
}

# Function to start the server process
start_server() {
    stop_server # Ensure no server is running before starting a new one

    BUILD_DIR="build/${OS}_${ARCH}_${BUILD_TYPE}"
    SERVER_BINARY="${BUILD_DIR}/loreforge"

    if [ ! -f "$SERVER_BINARY" ]; then
        echo "Error: Server binary not found at $SERVER_BINARY"
        echo "Please build the project first for this configuration."
        exit 1
    fi

    echo "Starting server: $SERVER_BINARY"
    nohup "$SERVER_BINARY" > "$LOG_FILE" 2>&1 &
    echo $! > "$PID_FILE"

    echo "Server starting in background with PID $(cat $PID_FILE)."
    echo "Waiting for server to initialize... (log: $LOG_FILE)"

    if timeout "$START_TIMEOUT"s grep -q "$SERVER_READY_MESSAGE" <(tail -f "$LOG_FILE"); then
        echo "Server started successfully."
        tail -n 5 "$LOG_FILE"
    else
        echo "Error: Server failed to start within $START_TIMEOUT seconds."
        cat "$LOG_FILE"
        stop_server
        exit 1
    fi
}

# --- Main Logic ---
COMMAND="start" # Default command

# Parse command-line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --os) OS="$2"; shift ;;
        --arch) ARCH="$2"; shift ;;
        --build) BUILD_TYPE="$2"; shift ;;
        -h|--help) usage; exit 0 ;;
        start|stop) COMMAND="$1" ;;
        *) echo "Unknown parameter passed: $1"; usage; exit 1 ;;
    esac
    shift
done

case $COMMAND in
    start)
        start_server
        ;;
    stop)
        stop_server
        ;;
esac

exit 0