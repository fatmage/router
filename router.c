#include "router.h"

int main() {

    // Establish neighbours
    uint32_t n_num;
    scanf("%d", &n_num);
    neighbour_t *neighbours = (neighbour_t*)malloc(n_num*sizeof(neighbour_t));
    uint32_t vec_len = n_num;
    route_t *dist_vec = (route_t*)malloc(n_num*(sizeof(route_t)));

    for (unsigned int i = 0; i < n_num; i++) {
        char cidr[19] = {};
        uint32_t dist;
        scanf("%s distance %d", cidr, &dist);

        int slash = strcspn(cidr, "/");
        cidr[slash] = '\0';
        if (inet_pton(AF_INET,cidr,&neighbours[i].network_ip) <= 0) {
            fprintf(stderr, "inet_pton error: [%s]\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // netowrk_ip and network_mask describe a neighbouring subnet in CIDR notation. 
        neighbours[i].network_ip = ntohl(neighbours[i].network_ip);
        neighbours[i].network_mask = atoi((char*)(cidr+slash+1));
        neighbours[i].distance = dist;
        neighbours[i].reachable = REACHABLE;
    }

    // Sort neighbours by network_mask in descending order 
    for (unsigned int i = 0; i < n_num; i++) {
        for (unsigned int j = 0; j < n_num - 1 - i; j++) {
            if (neighbours[j].network_mask < neighbours[j+1].network_mask) {
                neighbour_t tmp = neighbours[j];
                neighbours[j] = neighbours[j+1];
                neighbours[j+1] = tmp;
            }
        }
    }

    // Establish starting distance vector
    for (unsigned int i = 0; i < n_num; i++) {
        dist_vec[i].distance = neighbours[i].distance;
        dist_vec[i].path_ip = 0;
        dist_vec[i].network_ip = NETWORK(neighbours[i].network_ip,neighbours[i].network_mask);
        dist_vec[i].network_mask = neighbours[i].network_mask;
        dist_vec[i].directness = DIRECTLY;
        dist_vec[i].last_update = 0;
    }

    // After we established the starting state of the distance vector we start sending
    // it to neighbouring routers and listening for information sent from them to us.
    int sendfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (sendfd < 0) {
        fprintf(stderr, "socket error: [%s]\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    int broadcast = 1;
    if (setsockopt(sendfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        fprintf(stderr, "setsockopt error: [%s]\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int recfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (recfd < 0) {
        fprintf(stderr, "socket error: [%s]\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    broadcast = 1;
    if (setsockopt(recfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        fprintf(stderr, "setsockopt error: [%s]\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in recaddr;
    memset(&recaddr,0,sizeof(recaddr));

    recaddr.sin_family = AF_INET;
    recaddr.sin_addr.s_addr = INADDR_ANY;
    recaddr.sin_port = htons(54321);
    if (bind(recfd,(struct sockaddr *)&recaddr,sizeof(recaddr)) < 0) {
        fprintf(stderr, "bind error: [%s]\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fd_set dsc;
    FD_ZERO(&dsc);
    FD_SET(recfd, &dsc);

    // Program loop
    while(1) {

        print_vec(dist_vec, vec_len);

        for (unsigned int i = 0; i < vec_len; i++) {   
            dist_vec[i].last_update += 1;
        }
 
        // Send information to neighbouring routers.
        for (unsigned int i = 0; i < n_num; i++) {
            // Create and send datagrams containing a network in CIDR notation and our distance to it
            // to subnets connected directly with our device.
            uint32_t broadcast_ip = BROADCAST(neighbours[i].network_ip, neighbours[i].network_mask);
                
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(54321);
            addr.sin_addr.s_addr = htonl(broadcast_ip);    

            char buffer[9] = {};
            int32_t dist_ind = find_in_vec(dist_vec, vec_len, NETWORK(neighbours[i].network_ip,neighbours[i].network_mask), neighbours[i].network_mask);

            for (unsigned int j = 0; j < vec_len; j++) {

                *(uint32_t*)buffer = htonl(dist_vec[j].network_ip);
                buffer[4] = dist_vec[j].network_mask;
                if (dist_vec[j].distance < INFINITY)
                    *(uint32_t*)((long)buffer + 5) = htonl(dist_vec[j].distance);
                else
                    *(uint32_t*)((long)buffer + 5) = UINT32_MAX;

                if(sendto(sendfd, buffer, 9, 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0) {
                    // Failed to send, neighbour not in the distance vector.
                    if (dist_ind == -1)
                        break;
                    
                    // Failes to send, neighbour in the distance vector - set unreachable
                    dist_vec[i].distance = INFINITY;
                    for (unsigned int k = 0; k < vec_len; k++) {
                        if (NETWORK(dist_vec[k].path_ip, neighbours[i].network_mask) == 
                            NETWORK(neighbours[i].network_ip, neighbours[i].network_mask)) {
                            dist_vec[k].distance = INFINITY;
                        }
                    }
                    break;
                } else {
                    // Succeded to send, neighbour not in the distance vector - we add it
                    if (dist_ind == -1) {
                        route_t new_entry;
                        new_entry.directness = DIRECTLY;
                        new_entry.distance = neighbours[i].distance;
                        new_entry.network_ip = NETWORK(neighbours[i].network_ip,neighbours[i].network_mask);
                        new_entry.network_mask = neighbours[i].network_mask;
                        new_entry.path_ip = 0;
                        new_entry.last_update = 0;
                        
                        vec_len++;
                        dist_vec = realloc(dist_vec, vec_len*sizeof(route_t));
                        dist_vec[vec_len-1] = new_entry;
                        dist_ind = vec_len-1;
                    } else {
                    // Succeded to send, neighbour in the distance vector

                    // If the direct distance isn't longer than current distance we set the route
                    // in the distance vector to go to the net directly
                        if(neighbours[i].distance <= dist_vec[dist_ind].distance) {
                            dist_vec[dist_ind].directness = DIRECTLY;
                            dist_vec[dist_ind].distance = neighbours[i].distance;
                            dist_vec[dist_ind].network_ip = NETWORK(neighbours[i].network_ip,neighbours[i].network_mask);
                            dist_vec[dist_ind].network_mask = neighbours[i].network_mask;
                            dist_vec[dist_ind].path_ip = 0;
                        }
                        dist_vec[dist_ind].last_update = 0;
                    }
                }
            }
        }        



        struct timeval timeout;
        timeout.tv_sec  = TURN;
        timeout.tv_usec = 0;

        // Listen for information from neighbouring routers for the duration of one turn.
        FD_ZERO(&dsc);
        FD_SET(recfd, &dsc);

        while (select(recfd+1, &dsc, NULL, NULL, &timeout)) {

            struct sockaddr_in 	sender;	
		    socklen_t 			sender_len = sizeof(sender);
		    uint8_t 			data[9] = {};

            ssize_t p_len = recvfrom(
                recfd,
                data,
                9,
                MSG_DONTWAIT,
                (struct sockaddr*)&sender,
                &sender_len);

            if (p_len != 9)
                continue;

            // Parse packet
            uint32_t target_ip = ntohl(*(uint32_t*)data);
            uint8_t  target_mask = data[4];
            uint32_t target_dist = ntohl(*(uint32_t*)(data + 5));
            int32_t target_index = find_in_vec(dist_vec,vec_len,target_ip,target_mask);
            uint32_t sender_ip = ntohl(sender.sin_addr.s_addr);
            int32_t neighbour_index = find_in_neigh(neighbours,n_num,sender_ip);


            // The sender of the packet is not in our array of interfaces, ignore it.
            if (neighbour_index == -1)
                continue;
        
            // We are the sender, ignore it.
            if (own_ip(neighbours,n_num,sender_ip))
                continue;

            uint32_t new_dist; 
            if (target_dist >= INFINITY)
                new_dist = INFINITY;
            else
                new_dist = target_dist + neighbours[neighbour_index].distance;

            // If received distance to target network is smaller than INFINITY and the network isn't present
            // in the distance vector we add it to the vector.
            if (target_index == -1 && new_dist < INFINITY) {
                vec_len++;
                dist_vec = realloc(dist_vec, sizeof(route_t)*vec_len);
                dist_vec[vec_len-1].directness   = INDIRECTLY;
                dist_vec[vec_len-1].network_ip   = target_ip;
                dist_vec[vec_len-1].network_mask = target_mask;
                dist_vec[vec_len-1].path_ip = sender_ip;
                dist_vec[vec_len-1].distance = new_dist;
                dist_vec[vec_len-1].last_update = 0;
            } else {

                // If current route to target network goes through the sender of the packet, we update the distance
                if (sender_ip == dist_vec[target_index].path_ip) {
                    dist_vec[target_index].distance = new_dist;
                    if (new_dist < INFINITY)
                        dist_vec[target_index].last_update = 0;
                // If the route contained in the packet is shorter than our current route we update it.
                } else {
                    if (new_dist < dist_vec[target_index].distance) {
                        dist_vec[target_index].distance = new_dist;
                        dist_vec[target_index].path_ip = sender_ip;
                        dist_vec[target_index].directness = INDIRECTLY;

                        if (new_dist < INFINITY)
                            dist_vec[target_index].last_update = 0;
                    }

                }
            }

        }

        for (unsigned int i = 0; i < vec_len; i++) {
            if (dist_vec[i].last_update > 3) {
                vec_len--;
                if(i == vec_len) {
                    dist_vec = realloc(dist_vec,sizeof(route_t)*vec_len);

                } else {
                    for (unsigned int j = i; j < vec_len; j++) {
                        dist_vec[j] = dist_vec[j+1];
                    }
                    dist_vec = realloc(dist_vec,sizeof(route_t)*vec_len);
                    i--;
                }
            }
        }
    }

    free(neighbours);
    free(dist_vec);
    return EXIT_SUCCESS;
}
