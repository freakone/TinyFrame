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
#define TF_PARSER_TIMEOUT_TICKS 10
// Whether to use mutex - requires you to implement TF_ClaimTx() and TF_ReleaseTx()
#define TF_USE_MUTEX 1
// Error reporting function. To disable debug, change to empty define
#define TM_Error(format, ...) printf("[TM] " format "\n", ##__VA_ARGS__)

//------------------------- End of user config ------------------------------

#endif //TF_CONFIG_H
