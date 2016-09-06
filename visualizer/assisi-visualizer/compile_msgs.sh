#!/bin/bash
# Build python classes for parsing protobuf messages

echo "Starting the protoc compiler!"
protoc -I=msg --cpp_out=src/msg msg/*.proto
echo "Moving headers!"
mv src/msg/*.pb.h include/msg

