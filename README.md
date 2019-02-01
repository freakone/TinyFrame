# TinyModbus

First, easly portable ModBus RTU master library.

# Test

- Start com0com and bridge two com ports
- Start ModRSsim2 modbus slave simulator (or any other one) and connect it to the first com port
- Set second com port settings in test.go, function serialBridge
- `go run .`

# WORK IN PROGRESS

Inspired by [TinyFrame by MightyPork](https://github.com/MightyPork/TinyFrame).
