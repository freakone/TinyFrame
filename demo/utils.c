//
// Created by MightyPork on 2017/10/15.
//

#include "utils.h"
#include <stdio.h>

// helper func for testing
void dumpFrame(const uint8_t *buff, uint8_t len)
{
    printf("--- frame begin ---\n");
    size_t i;
    for (i = 0; i < len; i++)
    {
        printf("%3u \033[94m%02X\033[0m", buff[i], buff[i]);
        if (buff[i] >= 0x20 && buff[i] < 127)
        {
            printf(" %c", buff[i]);
        }
        else
        {
            printf(" \033[31m.\033[0m");
        }
        printf("\n");
    }
    printf("--- end of frame ---\n");

    printf("\x1b[0m");
}

void dumpQueryFrameInfo(TM_QueryMsg *msg)
{
    printf("\033[33mFrame info\n"
           "  addr: %02Xh\n"
           "  function: %02Xh\n"
           "  register: %04Xh\n"
           "  data: %04Xh\n",
           msg->peer_address, msg->function, msg->register_address, msg->data);
    printf("\x1b[0m");
}

// void dumpFrameInfo(TM_ResponseMsg *msg)
// {
//     // printf("\033[33mFrame info\n"
//     //        "  type: %02Xh\n"
//     //        "  data: \"%.*s\"\n"
//     //        "   len: %u\n"
//     //        "    id: %Xh\033[0m\n\n",
//     //        msg->type, msg->len, msg->data, msg->len, msg->frame_id);
// }
