#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mqueue.h>
#include <string.h>

#define MSG_SIZE 20

struct packet_stat {
    char ip_from[16];
    char ip_to[16];
    char protocol;
    unsigned short port_f;
    unsigned short port_t;
    unsigned short total_len;
    unsigned short udp_len;
};

void print_stat(struct packet_stat *ps)
{
    printf("From [%s:%i]\tto [%s:%i]\n", ps->ip_from, ps->port_f, ps->ip_to,
           ps->port_t);
    printf("protocol: %i\tlen: %ib\tudp len: %ib\n\n",
           ps->protocol, ps->total_len, ps->udp_len);
}

// дешифорвка
void deshif(const unsigned int *buff, struct packet_stat *ps)
{
    // ip
    inet_ntop(AF_INET, &buff[0], ps->ip_from, 16);
    inet_ntop(AF_INET, &buff[1], ps->ip_to, 16);

    // port
    ps->port_f = (buff[2] & 0xFFFF0000) >> 16;
    ps->port_t = buff[2] & 0xFFFF;

    // протокол, размер (полный и udp)
    ps->protocol = (buff[3] & 0xFF0000) >> 16;
    ps->total_len = buff[3] & 0xFFFF;
    ps->udp_len = buff[4];
}

int main()
{
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 3000,
        .mq_msgsize = MSG_SIZE
    };
    mqd_t mq_id = mq_open("/packet-filter", O_RDONLY,
                          S_IRUSR | S_IWUSR, &attr);
    if (mq_id == -1) {
        perror("packet-filter is not start!");
        exit(EXIT_FAILURE);
    }

    char buff[20] = {0};

    // принимаем
    mq_receive(mq_id, buff, MSG_SIZE, NULL);

    // полчучение числа пакетов
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned short i, packets_count = atoi(buff);
    struct packet_stat ps;

    for (i = 0; i < packets_count; i++) {
        mq_receive(mq_id, buff, MSG_SIZE, NULL);
        deshif((unsigned int *)buff, &ps);
        print_stat(&ps);
    }
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned int total_size = atoi(buff);

    puts("==========================");
    printf("Packets catch: %i\n", packets_count);
    printf("Total size: %i\n", total_size);

    mq_close(mq_id);

    return 0;
}
