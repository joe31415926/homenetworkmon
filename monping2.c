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
       #include <pthread.h>
#include <linux/fb.h>
#include <sys/mman.h>
       #include <sys/ioctl.h>
       #include <unistd.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <unistd.h>


volatile unsigned char idx[10];

struct timespec t[10][256];

void *start_routine(void *p)
{
    int fd = open("/dev/fb0", O_RDWR);
    assert(fd > 0);
    
    struct fb_fix_screeninfo finfo;
    assert(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == 0);
    assert(finfo.smem_len == 1280 * 720 * 2);
    
    struct fb_var_screeninfo vinfo;
    assert(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0);
    assert(vinfo.xres == 1280);
    assert(vinfo.yres == 720);
    assert(vinfo.bits_per_pixel == 16);
    
    char *framebuffer = (char *) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(framebuffer != MAP_FAILED);
    close(fd);
    
    fd = open("/home/pi/homenetworkmon/digits", O_RDONLY);
    assert(fd > 0);

    struct stat buf2;
    assert((fstat(fd, &buf2) == 0) && (buf2.st_size == 11 * 32 * 48 * 2));
    
    char *digits = (char *) mmap(0, buf2.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(digits != MAP_FAILED);
    close(fd);

    int lut[256];
    int i;
    for (i = 0; i < 256; i++) lut[i] = -1;
    lut['0'] = 0;
    lut['1'] = 1;
    lut['2'] = 2;
    lut['3'] = 3;
    lut['4'] = 4;
    lut['5'] = 5;
    lut['6'] = 6;
    lut['7'] = 7;
    lut['8'] = 8;
    lut['9'] = 9;
    lut['.'] = 10;
    lut[' '] = 0;
    

    
    while (1)
    {
        for (i = 0; i < sizeof(idx)/sizeof(idx[0]); i++)
        {
            unsigned char ii = idx[i] + 255;
            struct timespec now;
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &now) == 0);
            
            long deltal = (now.tv_nsec - t[i][ii].tv_nsec) / 100000000L;
            long deltah = now.tv_sec - t[i][ii].tv_sec;
            assert(deltah >= 0);
            while (deltah)
            {
                deltah--;
                deltal += 10;
            }            
            
            char buf[20];
            sprintf(buf, "%6ld", deltal);
            
            int d;
            for (d = 0; d < 6; d++)
            {
                int offset = lut[buf[d]];
                assert(offset >= 0);
                
                int j;
                for (j = 0; j < 48; j++)
                    memcpy(framebuffer + 1280 * 2 * (j + 50 * i) + 32 * 2 * d, digits + 11 * 32 * 2 * j + 32 * 2 * offset, 32 * 2);
            }

        }
    }
}

void child()
{
    FILE *logfile = fopen("/home/pi/ramdisk/monping2.log", "a");
    if (logfile)
    {
        fprintf(logfile, "restart %d\n", (int) time(NULL));
        fclose(logfile);
    }

    int i;
    for (i = 0; i < sizeof(idx)/sizeof(idx[0]); i++)
        idx[i] = 0;
        
    pthread_t thread;
    assert(pthread_create(&thread, NULL, start_routine, NULL) == 0);
        
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
                i = res[36] + 5;
                if ((i >= 0) && (i < sizeof(idx)/sizeof(idx[0])))
                {
                    assert(clock_gettime(CLOCK_MONOTONIC_RAW, &t[i][idx[i]]) == 0);
                    idx[i]++;
                }
            }
        }
        else
            assert(fds[0].revents == 0);
        
        if (fds[1].revents == POLLIN) {
            unsigned char res[300];
            int ressponse = recv(fds[1].fd, res, sizeof(res), 0);
            assert((ressponse > 0) && (ressponse < sizeof(res)));

            i = res[0] - '0';
            if ((i >= 0) && (i < sizeof(idx)/sizeof(idx[0])))
            {
                assert(clock_gettime(CLOCK_MONOTONIC_RAW, &t[i][idx[i]]) == 0);
                idx[i]++;
            }
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
    if (!fork()) mommy();
    return 0;
}
