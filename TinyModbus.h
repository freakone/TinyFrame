#ifndef TinyModbusH
#define TinyModbusH

//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL
#include <string.h>  // for memset()
//---------------------------------------------------------------------------

#include "TM_Config.h"
#define TM_VERSION "0.1.0"

#ifdef __cplusplus
extern "C"
{
#endif

    //---------------------------------------------------------------------------
    typedef enum
    {
        TM_READ_COILS = 0x01,
        TM_READ_INPUTS = 0x02,
        TM_READ_HOLDING_REGISTERS = 0x03,
        TM_READ_ANALOG_INPUTS = 0x04,
        TM_WRITE_COIL = 0x05,
        TM_WRITE_HOLDING_REGISTER = 0x06,
        TM_READ_EXCEPTION = 0x07,
        TM_WRITE_MULTIPLE_HOLDING_REGISTERS = 0x10
    } TM_COMMAND;

    typedef enum
    {
        TM_SLAVE = 0,
        TM_MASTER = 1,
    } TM_Peer;

    typedef struct TM_Query_
    {
        uint8_t peer_address;
        uint8_t function;
        uint16_t register_address;
        uint16_t data;
        uint16_t multipart_data[50];
        uint16_t multipart_length;
    } TM_QueryMsg;

    typedef struct TM_RawMessage_
    {
        uint8_t length;
        uint8_t data[TM_SENDBUF_LEN];
    } TM_RawMessage;

    typedef struct TM_ResponseMsg_
    {
        uint8_t peer_address;
        uint8_t function;
        uint16_t register_address;

        //response data
        uint16_t length;
        uint8_t *data;

        //error part
        bool is_error;
        uint8_t error_code;
        bool is_timeout;

        //if response device id mismatch
        bool wrong_response_peer_id;

    } TM_ResponseMsg;

    /** TinyFrame struct typedef */
    typedef struct TinyModbus_ TinyModbus;
    typedef void (*TM_Listener)(TinyModbus *tf, TM_ResponseMsg *msg);

    TinyModbus *TM_Init(TM_Peer peer_bit);
    bool TM_InitStatic(TinyModbus *tf, TM_Peer peer_bit);
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
    bool TM_SendMultipartSimple(TinyModbus *tf, uint8_t address, uint8_t function, uint16_t register_address, uint16_t *data, uint16_t length);
    bool TM_Send(TinyModbus *tf, TM_QueryMsg *msg);

    // ---------------------------------- INTERNAL ----------------------------------
    enum TM_State_
    {
        TMState_Idle = 0,
        TMState_WAITING_FOR_RESPONSE,
        TMState_DATA,
        TMState_CHECKSUM
    };

    struct TM_GenericListener_
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
        uint8_t dataReceive[TM_MAX_PAYLOAD_RX]; //!< Data byte buffer

        /* --- Callbacks --- */
        struct TM_GenericListener_ generic_listeners[TM_MAX_LISTENERS];
        uint8_t count_generic_lst;
    };

    // For implementation in users codebase
    extern void TM_WriteImpl(TinyModbus *tf, uint8_t *buff, uint32_t len);
    extern bool TM_ClaimTx(TinyModbus *tf);
    extern void TM_ReleaseTx(TinyModbus *tf);

#ifdef __cplusplus
}
#endif

#endif