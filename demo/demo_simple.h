#ifndef TF_DEMO_H
#define TF_DEMO_H

#include "../TinyModbus.h"
#include "utils.h"

void TM_WriteImpl(TinyModbus *tf, const uint8_t *buff, uint32_t len);
bool TM_ClaimTx(TinyModbus *tf);
void TM_ReleaseTx(TinyModbus *tf);

#endif
