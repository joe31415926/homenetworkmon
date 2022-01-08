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

void child()
{
    FILE *logfile = fopen("/home/pi/ramdisk/monping.log", "a");
    if (logfile)
    {
        fprintf(logfile, "restart %d\n", (int) time(NULL));
        fclose(logfile);
    }

    struct pollfd fds;
    fds.fd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    assert(fds.fd != -1);
    fds.events = POLLIN | POLLPRI;
    
    while (1)
    {
        assert ((poll(&fds, 1, -1) == 1) && (fds.revents == POLLIN));
        
        unsigned char res[100];
        int ressponse = recv(fds.fd, res, sizeof(res), 0);
        assert((ressponse > 0) && (ressponse < sizeof(res)));
        
        struct timespec receive_time;
        assert(clock_gettime(CLOCK_REALTIME, &receive_time) == 0);
        
        char buffer[32];
        int i;
        for(i = 0; i < 10; i++)
            sprintf(buffer + i * 2, "%02x", res[38 + i]);
            
        long message_nanoseconds = atol(buffer + 11);
        buffer[11] = '\0';
        long message_seconds = atol(buffer + 1);
        buffer[1] = '\0';
        long message_destination = atol(buffer + 0);
        
        long delta = (receive_time.tv_sec - message_seconds) * 1000 + (receive_time.tv_nsec - message_nanoseconds) / 1000000;
        
        int fd = open("/home/pi/ping.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
        assert(fd > 0);
        
        struct {
            uint32_t timestamp;
            uint16_t destination;
            uint16_t durationms;
        } record = {
            receive_time.tv_sec,
            message_destination,
            delta
        };
        
        assert(write(fd, &record, sizeof(record)) == sizeof(record));
        close(fd);
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
    if (!fork()) mommy();
    return 0;
}
