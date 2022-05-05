#include "router.h"


void print_vec(route_t *vec, int v_len) {
     
    for (int i = 0; i < v_len; i++) {
        char ip[20] = {};
        char path_ip[20] = {};
        uint32_t ip_netorder = htonl(vec[i].network_ip);
        uint32_t path_netorder = htonl(vec[i].path_ip);
        inet_ntop(AF_INET,&ip_netorder,ip,20);
        if (vec[i].distance < INFINITY) {
            printf("%s/%d distance %d ", ip, vec[i].network_mask, vec[i].distance);
        } else {
            printf("%s/%d unreachable ", ip, vec[i].network_mask);
        }
        if (vec[i].directness == DIRECTLY) {
            printf("connected directly\n");
        } else {
            inet_ntop(AF_INET,&path_netorder,path_ip,20);
            printf("via %s\n", path_ip);
        }
    } 
    printf("\n");
}

int32_t find_in_vec(route_t *vec, int v_len, uint32_t ip, uint8_t mask) {

    for (int i = 0; i < v_len; i++) {
        if (vec[i].network_ip == ip && vec[i].network_mask == mask)
            return i;
    }
    return -1;
}

int32_t find_in_neigh(neighbour_t *vec, int v_len, uint32_t ip) {

    for (int i = 0; i < v_len; i++) {
        if (NETWORK(ip,vec[i].network_mask) == NETWORK(vec[i].network_ip,vec[i].network_mask))
            return i;
    }

    return -1;
}

uint8_t own_ip(neighbour_t *vec, int v_len, uint32_t ip) {
    for (int i = 0; i < v_len; i++) {
        if (ip == vec[i].network_ip)
            return 1;
    }
    return 0;
}




