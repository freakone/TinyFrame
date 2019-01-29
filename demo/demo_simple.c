#include "demo_simple.h"

void TM_WriteImpl(TinyModbus *tf, const uint8_t *buff, uint32_t len)
{
    dumpFrame(buff, len);
}

bool TM_ClaimTx(TinyModbus *tf)
{
    printf("--- CLAIM TX --- \n");
    return true;
}

void TM_ReleaseTx(TinyModbus *tf)
{
    printf("--- RELEASE TX --- \n");
}

int main()
{
    TinyModbus *tm = TM_Init(TM_MASTER);

    TM_QueryMsg msg;
    TM_ClearQueryMsg(&msg);
    msg.peer_address = 1;
    msg.function = 6;
    msg.register_address = 0x2000;
    msg.data = 0x04;

    dumpQueryFrameInfo(&msg);

    TM_Send(tm, &msg, 10);

    return 1;
}