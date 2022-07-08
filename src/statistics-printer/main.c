#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mqueue.h>

// размер сообщения = int + short
#define MSG_SIZE 6

int main()
{
    struct mq_attr attr = {
        .mq_maxmsg = 10,
        .mq_msgsize = MSG_SIZE
    };
    mqd_t mq_id = mq_open("/packet-filter", O_RDONLY,
                          S_IRUSR | S_IWUSR, &attr);
    if (mq_id == -1) {
        perror("packet-filter is not start!");
        exit(EXIT_FAILURE);
    }

    char buff[MSG_SIZE] = {0};

    // принимаем
    mq_receive(mq_id, buff, MSG_SIZE, NULL);

    // полчучение числа пакетов
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned short packets_count = atoi(buff);
    mq_receive(mq_id, buff, MSG_SIZE, NULL);
    unsigned int total_bytes = atoi(buff);

    puts("==========================");
    printf("Packets catch: %i\n", packets_count);
    printf("Total size: %i\n", total_bytes);

    mq_close(mq_id);

    exit(EXIT_SUCCESS);
}
