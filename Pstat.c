#include "Pstat.h"

// заголовки ip и фрейма
#define FRAME_HSIZE 14
#define IP_HSIZE (FRAME_HSIZE + sizeof(struct iphdr))

unsigned int *Pocket_info (const char *buff)
{

    unsigned int *rez = malloc(5 * sizeof(unsigned int));

    // ip адреса
    struct iphdr *ip = (struct iphdr *)(buff + FRAME_HSIZE);
    rez[0] = ip->saddr;
    rez[1] = ip->daddr;

    //порты
    struct udphdr *udp = (struct udphdr *)(buff + IP_HSIZE);
    rez[2] = (ntohs(udp->source) << 16) + ntohs(udp->dest);

    //прочие данные
    rez[3] = (ip->protocol << 16) + ntohs(ip->tot_len);
    rez[4] = ntohs(udp->uh_ulen);

    return rez;
}

void Free_Pocket_info (struct Pockets_info *p_i)
{
    int i;
    for (i = 0; i < MAX_POCKET_COUNT; i++) {
        free(p_i->p_info[i]);
    }
}

