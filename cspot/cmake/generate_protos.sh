#!/bin/bash
cd cpp-reflection
go build -o protoc-gen-cpprefl .
cd ..
mkdir protos
protoc --plugin=protoc-gen-cpprefl=cpp-reflection/protoc-gen-cpprefl --cpprefl_out protos/ protocol/*.proto --proto_path protocol/