#ifndef TinyModbusH
#define TinyModbusH

//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL
#include <string.h>  // for memset()
//---------------------------------------------------------------------------

#include "TM_Config.h"

//---------------------------------------------------------------------------

/** Peer bit enum (used for init) */
typedef enum
{
    TM_SLAVE = 0,
    TM_MASTER = 1,
} TM_Peer;

/** Data structure for sending / receiving messages */
typedef struct TM_Query_
{
    uint8_t peer_address;
    uint8_t function;
    uint16_t register_address;
    uint16_t data;
} TM_QueryMsg;

typedef struct TM_RawMessage_
{
    uint8_t length;
    uint8_t data[TF_SENDBUF_LEN];
} TM_RawMessage;

/** Data structure for sending / receiving messages */
typedef struct TM_ResponseMsg_
{
    uint8_t peer_address;
    uint8_t function;
    uint16_t length;
    const uint8_t *data;
} TM_ResponseMsg;

/** TinyFrame struct typedef */
typedef struct TinyModbus_ TinyModbus;

/**
 * TinyFrame Type Listener callback
 *
 * @param tf - instance
 * @param msg - the received message, userdata is populated inside the object
 * @return listener result
 */
typedef void (*TM_Listener)(TinyModbus *tf, TM_ResponseMsg *msg);

// ---------------------------------- INIT ------------------------------

/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc).
 *
 * The field .userdata (or .usertag) can be used to identify different instances
 * in the TF_WriteImpl() function etc. Set this field after the init.
 *
 * This function is a wrapper around TF_InitStatic that calls malloc() to obtain
 * the instance.
 *
 * @param tf - instance
 * @param peer_bit - peer bit to use for self
 * @return TF instance or NULL
 */
TinyModbus *TM_Init(TM_Peer peer_bit);

/**
 * Initialize the TinyFrame engine using a statically allocated instance struct.
 *
 * The .userdata / .usertag field is preserved when TF_InitStatic is called.
 *
 * @param tf - instance
 * @param peer_bit - peer bit to use for self
 * @return success
 */
bool TM_InitStatic(TinyModbus *tf, TM_Peer peer_bit);

/**
 * De-init the dynamically allocated TF instance
 *
 * @param tf - instance
 */
void TM_DeInit(TinyModbus *tf);

static inline void TM_ClearResponseMsg(TM_ResponseMsg *msg)
{
    memset(msg, 0, sizeof(TM_ResponseMsg));
}

static inline void TM_ClearQueryMsg(TM_QueryMsg *msg)
{
    memset(msg, 0, sizeof(TM_QueryMsg));
}

// ---------------------------------- API CALLS --------------------------------------
TM_RawMessage TM_ConstructModbusBody(TM_QueryMsg *msg);
uint16_t TM_CRC16(uint8_t *data, uint16_t N);
void TM_Accept(TinyModbus *tf, const uint8_t *buffer, uint32_t count);
void TM_AcceptChar(TinyModbus *tf, uint8_t c);
void TM_Tick(TinyModbus *tf);
void TM_ResetParser(TinyModbus *tf);

// ---------------------------- MESSAGE LISTENERS -------------------------------

bool TM_AddGenericListener(TinyModbus *tf, TM_Listener cb);
bool TM_RemoveGenericListener(TinyModbus *tf, TM_Listener cb);

// ---------------------------- FRAME TX FUNCTIONS ------------------------------

bool TM_SendSimple(TinyModbus *tf, uint8_t address, uint8_t function, uint16_t register_address, uint16_t data);
bool TM_Send(TinyModbus *tf, TM_QueryMsg *msg);

// ---------------------------------- INTERNAL ----------------------------------
// This is publicly visible only to allow static init.

enum TM_State_
{
    TMState_SOF = 0,
    TMState_DATA,
    TMState_CHECKSUM
};

struct TF_GenericListener_
{
    TM_Listener fn;
};

/**
 * Frame parser internal state.
 */
struct TinyModbus_
{
    /* Parser state */
    TM_Peer peer_bit;
    enum TM_State_ state;
    uint16_t parser_timeout_ticks;

    TM_QueryMsg query;

    uint16_t lengthReceive;
    uint8_t dataReceive[TF_MAX_PAYLOAD_RX]; //!< Data byte buffer

    /* --- Callbacks --- */
    struct TF_GenericListener_ generic_listeners[TF_MAX_GEN_LST];
    uint8_t count_generic_lst;
};

// For implementation in users codebase
extern void TM_WriteImpl(TinyModbus *tf, uint8_t *buff, uint32_t len);
extern bool TM_ClaimTx(TinyModbus *tf);
extern void TM_ReleaseTx(TinyModbus *tf);

#endif
