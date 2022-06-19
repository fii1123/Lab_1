#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mqueue.h>
#include <string.h>

#define MSG_SIZE 20

struct Pocket_stat {
    char ip_from[16];
    char ip_to[16];
    char protocol;
    unsigned short port_f;
    unsigned short port_t;
    unsigned short total_len;
    unsigned short udp_len;
};

void Print_stat(struct Pocket_stat *Ps)
{
    printf("From [%s:%i]\tto [%s:%i]\n", Ps->ip_from, Ps->port_f, Ps->ip_to,
           Ps->port_t);
    printf("protocol: %i\tlen: %i\tudp len: %i\n\n",
           Ps->protocol, Ps->total_len, Ps->udp_len);
}

// дешифорвка
void Deshif(const unsigned int *buff, struct Pocket_stat *Ps)
{
    // ip
    inet_ntop(AF_INET, &buff[0], Ps->ip_from, 16);
    inet_ntop(AF_INET, &buff[1], Ps->ip_to, 16);

    // port
    Ps->port_f = (buff[2] & 0xFFFF0000) >> 16;
    Ps->port_t = buff[2] & 0xFFFF;

    // протокол, размер (полный и udp)
    Ps->protocol = (buff[3] & 0xFF0000) >> 16;
    Ps->total_len = buff[3] & 0xFFFF;
    Ps->udp_len = buff[4];
}

int main()
{
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 3000,
        .mq_msgsize = MSG_SIZE
    };
    mqd_t mq_id = mq_open("/LisIf", O_RDONLY,
                          S_IRUSR | S_IWUSR, &attr);
    if (mq_id == -1) {
        perror("LisIf is not start!");
        exit(EXIT_FAILURE);
    }

    char buff[20] = {0};

    // принимаем
    mq_receive(mq_id, buff, MSG_SIZE, NULL);

    // полчучение числа пакетов
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned short i, pockets_count = atoi(buff);
    struct Pocket_stat Ps;

    for (i = 0; i < pockets_count; i++){
        mq_receive(mq_id, buff, MSG_SIZE, NULL);
        Deshif((unsigned int *)buff, &Ps);
        Print_stat(&Ps);
    }
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned int total_size = atoi(buff);

    puts("==========================");
    printf("Pockets catch: %i\n", pockets_count);
    printf("Total size: %i\n", total_size);

    mq_close(mq_id);

    return 0;
}
