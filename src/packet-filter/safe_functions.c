#include "safe_functions.h"

int create_raw_socket()
{
    int s = socket(AF_PACKET, SOCK_RAW, IPPROTO_UDP);
    if (s == -1) {
        perror("socket error!");
        exit(EXIT_FAILURE);
    }
    return s;
}

void set_index_from_ireq(int s, struct ifreq *if_req)
{
    if (ioctl(s, SIOCGIFINDEX, if_req, sizeof(if_req)) == -1) {
        perror("can't find device!");
        exit(EXIT_FAILURE);
    }
}

void bind_socket(const int s, struct ifreq *if_req)
{
    struct sockaddr_ll sa = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = if_req->ifr_ifindex
    };
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind_socket error!");
        exit(EXIT_FAILURE);
    }
}

mqd_t msg_queue_open(struct mq_attr *attr)
{
    mqd_t mq_id = mq_open("/packet-filter", O_CREAT | O_WRONLY,
                          S_IRUSR | S_IWUSR, attr);
    if (mq_id == -1) {
        perror("msg queue can't be create!");
        exit(EXIT_FAILURE);
    }
    return mq_id;
}
