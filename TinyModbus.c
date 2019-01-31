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
bool TM_AddGenericListener(TinyModbus *tf, TM_Listener cb)
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
bool TM_RemoveGenericListener(TinyModbus *tf, TM_Listener cb)
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

    if (tf->lengthReceive <= 4)
    {
        TM_Error("Unhandled message, message to short");
        return;
    }

    uint16_t crc = TM_CRC16(tf->dataReceive, tf->lengthReceive - 2);

    uint16_t crcFromMessage = tf->dataReceive[tf->lengthReceive - 2];
    crcFromMessage |= tf->dataReceive[tf->lengthReceive - 1] << 8;

    printf("CRC CHECK %04X %04X\n", crc, crcFromMessage);
    if (crc != crcFromMessage)
    {
        TM_Error("Unhandled message, wrong checksum");
        return;
    }

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
}

//endregion Listeners

//region Parser

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

/** Handle a received byte buffer */
void TM_Accept(TinyModbus *tf, const uint8_t *buffer, uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++)
    {
        TM_AcceptChar(tf, buffer[i]);
    }
}

/** Handle a received char - here's the main state machine */
void TM_AcceptChar(TinyModbus *tf, unsigned char c)
{
    // Parser timeout - clear
    if (tf->parser_timeout_ticks >= TF_PARSER_TIMEOUT_TICKS)
    {
        if (tf->state != TMState_SOF)
        {
            TF_HandleReceivedMessage(tf);
            tf->state = TMState_SOF;
        }
    }

    tf->parser_timeout_ticks = 0;

    if (tf->state == TMState_SOF)
    {
        tf->lengthReceive = 0;
        tf->state = TMState_DATA;
    }

    if (tf->lengthReceive + 1 < TF_MAX_PAYLOAD_RX)
    {
        tf->dataReceive[tf->lengthReceive++] = c;
    }
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

bool TM_Send(TinyModbus *tf, TM_QueryMsg *msg)
{
    TF_TRY(TM_ClaimTx(tf));
    tf->query = *msg;

    TM_RawMessage raw = TM_ConstructModbusBody(msg);
    TM_WriteImpl(tf, (uint8_t *)raw.data, raw.length);
    TM_ReleaseTx(tf);

    return true;
}

bool TM_SendSimple(TinyModbus *tf, uint8_t address, uint8_t function, uint16_t register_address, uint16_t data)
{
    TM_QueryMsg msg;
    TM_ClearQueryMsg(&msg);
    msg.peer_address = address;
    msg.function = function;
    msg.register_address = register_address;
    msg.data = data;
    return TM_Send(tf, &msg);
}

//endregion Sending API funcs

/** Timebase hook - for frame begin/end handling */
void TM_Tick(TinyModbus *tf)
{
    // increment parser timeout (timeout is handled when receiving next byte)
    if (tf->parser_timeout_ticks < TF_PARSER_TIMEOUT_TICKS)
    {
        tf->parser_timeout_ticks++;
    }
}
