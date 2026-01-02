#!/bin/bash
MCP_SERVER="./build/linux_x64_debug/mcp-server"

# Input JSON-RPC requests
INPUT=$(cat <<EOF
{"jsonrpc": "2.0", "method": "initialize", "params": {}, "id": 1}
{"jsonrpc": "2.0", "method": "tools/list", "params": {}, "id": 2}
{"jsonrpc": "2.0", "method": "tools/call", "params": {"name": "analyze_function", "arguments": {"file_path": "tests/cpp_test_files/sample.cpp", "function_name": "myFunction"}}, "id": 3}
EOF
)

# Run server and feed input
echo "$INPUT" | $MCP_SERVER > mcp_output.txt

# Display output
cat mcp_output.txt

# Simple checks
if grep -q "analyze_function" mcp_output.txt; then
    echo "PASS: tools/list contains analyze_function"
else
    echo "FAIL: tools/list missing analyze_function"
fi

if grep -q "Signature:" mcp_output.txt; then
    echo "PASS: analyze_function returned signature"
else
    echo "FAIL: analyze_function did not return signature"
fi
