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

void Bind(const int s, struct ifreq *if_req)
{
    struct sockaddr_ll sa = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = if_req->ifr_ifindex
    };
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("Bind error!");
        exit(EXIT_FAILURE);
    }
}