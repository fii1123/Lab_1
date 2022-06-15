#include "Errors.h"

int Raw_socket()
{
    int s = socket(AF_PACKET, SOCK_RAW, IPPROTO_UDP);
        if (s == -1) {
            perror("Socket error!");
            exit(EXIT_FAILURE);
        }
        return s;
}

void Ioctl(int s, struct ifreq *if_req)
{
    if (ioctl(s, SIOCGIFINDEX, if_req, sizeof(if_req)) == -1) {
        perror("ioctl error!");
        exit(EXIT_FAILURE);
    }
}

void Bind(const int s, struct sockaddr *sa, socklen_t len)
{
    if (bind(s, sa, len) == -1) {
        perror("Bind error!");
        exit(EXIT_FAILURE);
    }
}

mqd_t Mq_open()
{
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 1000,
        .mq_msgsize = 20
    };
    mqd_t mq_id = mq_open("/LisIf", O_CREAT | O_RDWR,
                          S_IRUSR | S_IWUSR, &attr);
    if (mq_id == -1) {
        perror("mq error!");
        exit(EXIT_FAILURE);
    }
    return mq_id;
}
