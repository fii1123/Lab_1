#ifndef PSTAT_H
#define PSTAT_H

#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define MAX_POCKET_COUNT 65535

// информация о пакете в бинарном виде
struct Pockets_info {
    unsigned int *p_info[MAX_POCKET_COUNT];
    unsigned short count;
};

unsigned int *Pocket_info (const char *buff);

void Free_Pocket_info (struct Pockets_info *p_i);

#endif // PSTAT_H
