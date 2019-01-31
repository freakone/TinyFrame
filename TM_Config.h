#ifndef TM_CONFIGH
#define TM_CONFIGH

#include <stdint.h>
#include <stdio.h>
//----------------------------- PARAMETERS ----------------------------------

// Maximum received payload size (static buffer)
// Larger payloads will be rejected.
#define TF_MAX_PAYLOAD_RX 1024
// Size of the sending buffer. Larger payloads will be split to pieces and sent
// in multiple calls to the write function. This can be lowered to reduce RAM usage.
#define TF_SENDBUF_LEN 128
// --- Listener counts - determine sizes of the static slot tables ---
// Listeners
#define TF_MAX_GEN_LST 10
// Timeout for receiving & parsing a frame
// ticks = number of calls to TF_Tick()
// set it to value that will be grater than 3.5 of uart sign
// for example:
// 9600bps -> 1 sign equals to 1000000us / 9600 = 105us
// lets assume TF_Tick is called once in 100us
// this should be defined > 4
#define TF_PARSER_TIMEOUT_TICKS 5
// Error reporting function. To disable debug, change to empty define
#define TM_Error(format, ...) printf("[TM] " format "\n", ##__VA_ARGS__)

//------------------------- End of user config ------------------------------

#endif //TF_CONFIG_H
