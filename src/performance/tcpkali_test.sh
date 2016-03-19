SERVER="localhost:7002"

tcpkali --nagle on \
        --connections 1 \
        --verbose 1 \
        --first-message-file first.bin \
        --message-file message.bin \
        $SERVER
