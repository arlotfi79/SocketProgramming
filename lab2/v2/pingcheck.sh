#!/bin/bash

# Check if all required arguments are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <server_ip> <server_port> <client_ip>"
    exit 1
fi

# Assign command-line arguments to variables
SERVER_IP=$1
SERVER_PORT=$2
CLIENT_IP=$3

# Run the client code 10 times and save the results
echo "# pingc.bin results" >> rtt_results.txt
for ((i=1; i<=10; i++)); do
    ../v1/pingc.bin "$SERVER_IP" "$SERVER_PORT" "$CLIENT_IP" | grep -oP 'RTT: \K[0-9.]+ milliseconds' >> rtt_results.txt
    sleep 1
done

# Ping the server IP and save results in the same file
echo "# /bin/ping results" >> rtt_results.txt
/bin/ping -c 10 "$SERVER_IP" >> rtt_results.txt

echo "pingc.bin and /bin/ping results saved in rtt_results.txt"