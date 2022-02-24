#include <stdio.h>
#include <unistd.h>     // fork(), sleep(), write(), close()
#include <arpa/inet.h>  // IPPROTO_ICMP
#include <assert.h>
#include <stdlib.h>     // aotl()
#include <sys/socket.h> // socket()
#include <fcntl.h>      // open()
#include <time.h>       // time()
#include <poll.h>       // poll()
#include <sys/wait.h>   // wait()
       #include <string.h>

void child()
{
    FILE *logfile = fopen("/home/pi/ramdisk/monping2.log", "a");
    if (logfile)
    {
        fprintf(logfile, "restart %d\n", (int) time(NULL));
        fclose(logfile);
    }

    struct pollfd fds[2];
    
    fds[0].fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    assert(fds[0].fd != -1);
    fds[0].events = POLLIN | POLLPRI;
    
    fds[1].fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(fds[1].fd != -1);
    fds[1].events = POLLIN | POLLPRI;
    
    int yes = 1;
    if (setsockopt(fds[1].fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0)
        return;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(fds[1].fd, (struct sockaddr *) &addr, sizeof(addr)) != 0)
        return;
    
    while (1)
    {
        assert(poll(fds, 2, -1) > 0);
        
        if (fds[0].revents == POLLIN) {
            unsigned char res[300];
            int ressponse = recv(fds[0].fd, res, sizeof(res), 0);
            assert((ressponse > 0) && (ressponse < sizeof(res)));
            
            if ((res[9] == 1) && (res[20] == 0) && (res[21] == 0))
            {
                struct timespec receive_time;
                assert(clock_gettime(CLOCK_MONOTONIC_RAW, &receive_time) == 0);
            
                printf("icmp %09d.%09d %d\n", receive_time.tv_sec, receive_time.tv_nsec, res[36]);
            }
        }
        else
            assert(fds[0].revents == 0);
        
        if (fds[1].revents == POLLIN) {
            unsigned char res[300];
            int ressponse = recv(fds[1].fd, res, sizeof(res), 0);
            assert((ressponse > 0) && (ressponse < sizeof(res)));

            struct timespec receive_time;
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &receive_time) == 0);
            
            printf("udp %09d.%09d %d\n", receive_time.tv_sec, receive_time.tv_nsec, res[0] - '0');
        }
        else
            assert(fds[1].revents == 0);
    }
}

void mommy()
{
    while (1)
    {
        if (!fork()) child();
        wait(NULL);
        sleep(1);    // wait for a second so we don't spin super quickly
    }
}

int main()
{
    child();
    return -1;
    if (!fork()) mommy();
    return 0;
}
