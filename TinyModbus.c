//---------------------------------------------------------------------------
#include "TinyModbus.h"
#include <stdlib.h> // - for malloc() if dynamic constructor is used
//---------------------------------------------------------------------------

// Helper macros
#define TF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TF_TRY(func)      \
    do                    \
    {                     \
        if (!(func))      \
            return false; \
    } while (0)

//region Init

/** Init with a user-allocated buffer */
bool TM_InitStatic(TinyModbus *tf, TM_Peer peer_bit)
{
    if (tf == NULL)
    {
        TM_Error("TM_InitStatic() failed, tf is null.");
        return false;
    }

    memset(tf, 0, sizeof(struct TinyModbus_));

    tf->peer_bit = peer_bit;
    return true;
}

/** Init with malloc */
TinyModbus *TM_Init(TM_Peer peer_bit)
{
    TinyModbus *tf = malloc(sizeof(TinyModbus));
    if (!tf)
    {
        TM_Error("TF_Init() failed, out of memory.");
        return NULL;
    }

    TM_InitStatic(tf, peer_bit);
    return tf;
}

/** Release the struct */
void TM_DeInit(TinyModbus *tf)
{
    if (tf == NULL)
        return;
    free(tf);
}
//endregion Init

//region Listeners

/** Clean up Generic listener */
static inline void cleanup_generic_listener(TinyModbus *tf, uint8_t i, struct TF_GenericListener_ *lst)
{
    lst->fn = NULL; // Discard listener
    if (i == tf->count_generic_lst - 1)
    {
        tf->count_generic_lst--;
    }
}

/** Add a new Generic listener. Returns 1 on success. */
bool TF_AddGenericListener(TinyModbus *tf, TM_Listener cb)
{
    uint8_t i;
    struct TF_GenericListener_ *lst;
    for (i = 0; i < TF_MAX_GEN_LST; i++)
    {
        lst = &tf->generic_listeners[i];
        // test for empty slot
        if (lst->fn == NULL)
        {
            lst->fn = cb;
            if (i >= tf->count_generic_lst)
            {
                tf->count_generic_lst = (uint8_t)(i + 1);
            }
            return true;
        }
    }

    TM_Error("Failed to add generic listener");
    return false;
}

/** Remove a generic listener by its function pointer. Returns 1 on success. */
bool TF_RemoveGenericListener(TinyModbus *tf, TM_Listener cb)
{
    uint8_t i;
    struct TF_GenericListener_ *lst;
    for (i = 0; i < tf->count_generic_lst; i++)
    {
        lst = &tf->generic_listeners[i];
        // test if live & matching
        if (lst->fn == cb)
        {
            cleanup_generic_listener(tf, i, lst);
            return true;
        }
    }

    TM_Error("Generic listener to remove not found");
    return false;
}

/** Handle a message that was just collected & verified by the parser */
static void TF_HandleReceivedMessage(TinyModbus *tf)
{
    uint8_t i;
    struct TF_GenericListener_ *glst;

    // Prepare message object
    TM_ResponseMsg msg;
    TM_ClearResponseMsg(&msg);
    msg.function = tf->query.register_address;
    msg.peer_address = tf->query.peer_address;
    msg.data = tf->dataReceive;
    msg.length = tf->lengthReceive;

    // Generic listeners
    for (i = 0; i < tf->count_generic_lst; i++)
    {
        glst = &tf->generic_listeners[i];

        if (glst->fn)
        {
            glst->fn(tf, &msg);
        }
    }

    TM_Error("Unhandled message, type %d", (int)msg.function);
}

//endregion Listeners

//region Parser

/** Handle a received byte buffer */
void TM_Accept(TinyModbus *tf, const uint8_t *buffer, uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++)
    {
        // TM_AcceptChar(tf, buffer[i]);
    }
}

/** Reset the parser's internal state. */
void TM_ResetParser(TinyModbus *tf)
{
    tf->state = TFState_SOF;
    // more init will be done by the parser when the first byte is received
}

// /** Handle a received char - here's the main state machine */
// void TM_AcceptChar(TinyModbus *tf, unsigned char c)
// {
//     // Parser timeout - clear
//     if (tf->parser_timeout_ticks >= TF_PARSER_TIMEOUT_TICKS)
//     {
//         if (tf->state != TFState_SOF)
//         {
//             TF_ResetParser(tf);
//             TM_Error("Parser timeout");
//         }
//     }
//     tf->parser_timeout_ticks = 0;

//     //@formatter:off
//     switch (tf->state)
//     {
//     case TFState_SOF:
//         if (c == TF_SOF_BYTE)
//         {
//             pars_begin_frame(tf);
//         }
//         break;

//     case TFState_ID:
//         CKSUM_ADD(tf->cksum, c);
//         COLLECT_NUMBER(tf->id, TF_ID)
//         {
//             // Enter LEN state
//             tf->state = TFState_LEN;
//             tf->rxi = 0;
//         }
//         break;

//     case TFState_LEN:
//         CKSUM_ADD(tf->cksum, c);
//         COLLECT_NUMBER(tf->len, TF_LEN)
//         {
//             // Enter TYPE state
//             tf->state = TFState_TYPE;
//             tf->rxi = 0;
//         }
//         break;

//     case TFState_TYPE:
//         CKSUM_ADD(tf->cksum, c);
//         COLLECT_NUMBER(tf->type, TF_TYPE)
//         {
// #if TF_CKSUM_TYPE == TF_CKSUM_NONE
//             tf->state = TFState_DATA;
//             tf->rxi = 0;
// #else
//             // enter HEAD_CKSUM state
//             tf->state = TFState_HEAD_CKSUM;
//             tf->rxi = 0;
//             tf->ref_cksum = 0;
// #endif
//         }
//         break;

//     case TFState_HEAD_CKSUM:
//         COLLECT_NUMBER(tf->ref_cksum, TF_CKSUM)
//         {
//             // Check the header checksum against the computed value
//             CKSUM_FINALIZE(tf->cksum);

//             if (tf->cksum != tf->ref_cksum)
//             {
//                 TM_Error("Rx head cksum mismatch");
//                 TF_ResetParser(tf);
//                 break;
//             }

//             if (tf->len == 0)
//             {
//                 // if the message has no body, we're done.
//                 TF_HandleReceivedMessage(tf);
//                 TF_ResetParser(tf);
//                 break;
//             }

//             // Enter DATA state
//             tf->state = TFState_DATA;
//             tf->rxi = 0;

//             CKSUM_RESET(tf->cksum); // Start collecting the payload

//             if (tf->len > TF_MAX_PAYLOAD_RX)
//             {
//                 TM_Error("Rx payload too long: %d", (int)tf->len);
//                 // ERROR - frame too long. Consume, but do not store.
//                 tf->discard_data = true;
//             }
//         }
//         break;

//     case TFState_DATA:
//         if (tf->discard_data)
//         {
//             tf->rxi++;
//         }
//         else
//         {
//             CKSUM_ADD(tf->cksum, c);
//             tf->data[tf->rxi++] = c;
//         }

//         if (tf->rxi == tf->len)
//         {
// #if TF_CKSUM_TYPE == TF_CKSUM_NONE
//             // All done
//             TF_HandleReceivedMessage(tf);
//             TF_ResetParser(tf);
// #else
//             // Enter DATA_CKSUM state
//             tf->state = TFState_DATA_CKSUM;
//             tf->rxi = 0;
//             tf->ref_cksum = 0;
// #endif
//         }
//         break;

//     case TFState_DATA_CKSUM:
//         COLLECT_NUMBER(tf->ref_cksum, TF_CKSUM)
//         {
//             // Check the header checksum against the computed value
//             CKSUM_FINALIZE(tf->cksum);
//             if (!tf->discard_data)
//             {
//                 if (tf->cksum == tf->ref_cksum)
//                 {
//                     TF_HandleReceivedMessage(tf);
//                 }
//                 else
//                 {
//                     TM_Error("Body cksum mismatch");
//                 }
//             }

//             TF_ResetParser(tf);
//         }
//         break;
//     }
//     //@formatter:on
// }

uint16_t TM_CRC16(uint8_t *data, uint16_t N)
{
    uint16_t reg, bit, yy, flag;
    yy = 0;
    reg = 0xffff;
    while (N-- > 0)
    {
        reg = reg ^ data[yy];
        yy = yy + 1;
        for (bit = 0; bit <= 7; bit++)
        {
            flag = reg & 0x0001;
            reg = reg >> 1;
            if (flag == 1)
                reg = reg ^ 0xa001;
        }
    }
    return (reg);
}

TM_RawMessage TM_ConstructModbusBody(TM_QueryMsg *msg)
{
    TM_RawMessage raw;
    raw.length = 0;
    raw.data[0] = msg->peer_address;
    raw.data[1] = msg->function;

    switch (msg->function)
    {
    default:
        raw.length = 8;
        raw.data[2] = msg->register_address >> 8;
        raw.data[3] = msg->register_address & 0xFF;
        raw.data[4] = msg->data >> 8;
        raw.data[5] = msg->data & 0xFF;

        uint16_t crc = TM_CRC16(raw.data, 6);
        raw.data[6] = crc & 0xFF;
        raw.data[7] = crc >> 8;
        break;
    }

    return raw;
}

/**
 * Send a message
 *
 * @param tf - instance
 * @param msg - message object
 * @param listener - ID listener, or NULL
 * @param timeout - listener timeout, 0 is none
 * @return true if sent
 */
bool TM_Send(TinyModbus *tf, TM_QueryMsg *msg, uint16_t timeout)
{
    TF_TRY(TM_ClaimTx(tf));
    tf->query = *msg;

    TM_RawMessage raw = TM_ConstructModbusBody(msg);
    TM_WriteImpl(tf, (const uint8_t *)raw.data, raw.length);
    TM_ReleaseTx(tf);

    return true;
}

//endregion Sending API funcs

/** Timebase hook - for timeouts */
void TM_Tick(TinyModbus *tf)
{
    // increment parser timeout (timeout is handled when receiving next byte)
    if (tf->parser_timeout_ticks < TF_PARSER_TIMEOUT_TICKS)
    {
        tf->parser_timeout_ticks++;
    }
}
