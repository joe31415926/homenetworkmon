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
#include <math.h>

// ffmpeg -y -i congrats.jpg -vcodec rawvideo -f rawvideo -s 1280x720 -pix_fmt rgb565 out
// ffmpeg -y -i digits.png -vcodec rawvideo -f rawvideo -pix_fmt rgb565 digits

#define BYTES_PER_PIXEL (2)

#define SCREEN_BUFFER_WIDTH_PIXELS (1280)
#define SCREEN_BUFFER_HEIGHT_PIXELS (720)
#define SCREEN_BUFFER_NUM_PIXELS (SCREEN_BUFFER_WIDTH_PIXELS * SCREEN_BUFFER_HEIGHT_PIXELS)
#define SCREEN_BUFFER_NUM_BYTES (SCREEN_BUFFER_NUM_PIXELS * BYTES_PER_PIXEL)
#define SCREEN_BUFFER_ROW_OFFSET_BYTES (SCREEN_BUFFER_WIDTH_PIXELS * BYTES_PER_PIXEL)

#define NUM_PROBE_POINTS (10)

#define NUM_DIGIT_IMAGES (12)
#define DIGIT_IMAGE_WIDTH_PIXELS (32)
#define DIGIT_IMAGES_ROW_OFFSET_PIXELS (NUM_DIGIT_IMAGES * DIGIT_IMAGE_WIDTH_PIXELS)
#define DIGIT_IMAGE_WIDTH_BYTES (DIGIT_IMAGE_WIDTH_PIXELS * BYTES_PER_PIXEL)
#define DIGIT_IMAGES_ROW_OFFSET_BYTES (NUM_DIGIT_IMAGES * DIGIT_IMAGE_WIDTH_BYTES)
#define DIGIT_IMAGE_HEIGHT_PIXELS (48)
#define DIGIT_IMAGES_SIZE_BYTES (DIGIT_IMAGES_ROW_OFFSET_BYTES * DIGIT_IMAGE_HEIGHT_PIXELS)
#define DIGIT_IMAGES_SIZE_PIXELS (DIGIT_IMAGES_ROW_OFFSET_PIXELS * DIGIT_IMAGE_HEIGHT_PIXELS)

#define NUM_FLASH_FRAMES (12)
#define FLASH_IMAGE_WIDTH_PIXELS (32)
#define FLASH_IMAGE_WIDTH_BYTES (FLASH_IMAGE_WIDTH_PIXELS * BYTES_PER_PIXEL)
#define FLASH_IMAGES_ROW_OFFSET_BYTES (NUM_FLASH_FRAMES * FLASH_IMAGE_WIDTH_PIXELS * BYTES_PER_PIXEL)
#define FLASH_IMAGE_HEIGHT_PIXELS (48)
#define FLASH_IMAGES_SIZE_BYTES (FLASH_IMAGES_ROW_OFFSET_BYTES * FLASH_IMAGE_HEIGHT_PIXELS)
#define FLASH_TIME_PER_FRAME (30)

#define NUMBER_STATUS_COLORS (3)
#define STATUS_PIXELS_FILLED (33)

typedef struct {
    int x;
    int y;
    int d;
    int frame;
    char buf[20];
} ui_data_str;

int lut[0x100]; // index is a char
char *framebuffer;
char *filldigits;
char *flash;
char *background;

void update(ui_data_str *d, long t, int refresh)
{
    if (refresh)
        memcpy(framebuffer, background, SCREEN_BUFFER_NUM_BYTES);
    
    int x_offset_bytes = 0;

    if (d->d)
    {
        char buf[20];
        sprintf(buf, "%4ld.%1ld", t / 1000 / 10, (t / 1000) % 10);

        int i;
        for (i = 0; i < 6; i++) // 6 characters in: "0000.0"
        {
 //           if (refresh || (buf[i] != d->buf[i]))
            {
                // every factor of 10X will increase the width by 32 pixels
                // don't forget: you can't take the log() of zero
                int off = t > 0 ? DIGIT_IMAGE_WIDTH_PIXELS * log(t) / M_LN10 : 0;

                // t = 100000 (off = 5 * DIGIT_IMAGE_WIDTH_PIXELS) should line up with the point between the 3rd and 4th characters (i = 2)
                int digitoff = (6 - i) * DIGIT_IMAGE_WIDTH_PIXELS;

                int stretch = off - digitoff;
                if (stretch < 0) stretch = 0;
                if (stretch > 32) stretch = 32;

                if (stretch == 0)
                {
                    int digitcolor = 0;
                    if (i == 1) digitcolor = 1;
                    if (i < 1) digitcolor = 2;
                    stretch = digitcolor * 33;
                }
                else
                {

                    if (t >= 100000)
                        stretch += 33;
                    if (t >= 1000000)
                        stretch += 33;

                }
                int offset = lut[buf[i]];
                assert(offset >= 0);

                int j;
                for (j = 0; j < 48; j++)
                    memcpy(framebuffer + SCREEN_BUFFER_ROW_OFFSET_BYTES * (d->y + j) + BYTES_PER_PIXEL * d->x + DIGIT_IMAGE_WIDTH_BYTES * i + x_offset_bytes,
                    filldigits + stretch * DIGIT_IMAGES_SIZE_BYTES + DIGIT_IMAGES_ROW_OFFSET_BYTES * j + DIGIT_IMAGE_WIDTH_BYTES * offset,
                    DIGIT_IMAGE_WIDTH_BYTES);
            }
        }

        memcpy(d->buf, buf, 20);

        x_offset_bytes = 6 * DIGIT_IMAGE_WIDTH_BYTES;
    }
    
    int frame = t / FLASH_TIME_PER_FRAME;
    if ((frame >= 0) && (frame < 2 * NUM_FLASH_FRAMES))        // As t increases, frame should first go 0 -> 11 and then go 11 -> 0.
    {
        if (frame >= NUM_FLASH_FRAMES)
            frame = 2 * NUM_FLASH_FRAMES - 1 - frame;

        if (refresh || (frame != d->frame))
        {
            int j;
            for (j = 0; j < FLASH_IMAGE_HEIGHT_PIXELS; j++)
                memcpy(framebuffer + SCREEN_BUFFER_ROW_OFFSET_BYTES * (d->y + j) + BYTES_PER_PIXEL * d->x + x_offset_bytes, flash + FLASH_IMAGES_ROW_OFFSET_BYTES * j + FLASH_IMAGE_WIDTH_BYTES * frame, FLASH_IMAGE_WIDTH_BYTES);
            d->frame = frame;
        }
    }

}

ui_data_str ui[NUM_PROBE_POINTS];

volatile unsigned char idx[NUM_PROBE_POINTS];

struct timespec t[NUM_PROBE_POINTS][256];

void *start_routine(void *p)
{
    int fd = open("/dev/fb0", O_RDWR);
    assert(fd > 0);

    struct fb_fix_screeninfo finfo;
    assert(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == 0);
    assert(finfo.smem_len == SCREEN_BUFFER_NUM_BYTES);

    struct fb_var_screeninfo vinfo;
    assert(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0);
    assert(vinfo.xres == SCREEN_BUFFER_WIDTH_PIXELS);
    assert(vinfo.yres == SCREEN_BUFFER_HEIGHT_PIXELS);
    assert(vinfo.bits_per_pixel == 16);

    framebuffer = (char *) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(framebuffer != MAP_FAILED);
    close(fd);

    fd = open("/home/pi/homenetworkmon/background", O_RDONLY);
    assert(fd > 0);

    struct stat st;
    assert((fstat(fd, &st) == 0) && (st.st_size == SCREEN_BUFFER_NUM_BYTES));
    
    background = (char *) mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(background != MAP_FAILED);
    close(fd);

    fd = open("/home/pi/homenetworkmon/digits", O_RDONLY);
    assert(fd > 0);

    assert((fstat(fd, &st) == 0) && (st.st_size == DIGIT_IMAGES_SIZE_BYTES));

    char *digits = (char *) mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(digits != MAP_FAILED);
    close(fd);

// green  0x07e0
// light green  0xbff7

// yellow 0xf7c2
// light yellow 0xfff7

// red    0xf800
// light red    0xface 11111 110111 11011 fefb

    const uint16_t color_white = 0xffff;
    uint16_t colors[NUMBER_STATUS_COLORS][2] = {{0x07e0, 0xbff7}, {0xf7c2, 0xfff7}, {0xf800, 0xfefb}};

    filldigits = malloc(DIGIT_IMAGES_SIZE_BYTES * NUMBER_STATUS_COLORS * STATUS_PIXELS_FILLED);
    assert(filldigits != NULL);

    int color_idx, offset_idx, row_idx, digit_idx, pixel_idx;
    for (color_idx = 0; color_idx < NUMBER_STATUS_COLORS; color_idx++)
        for (offset_idx = 0; offset_idx < STATUS_PIXELS_FILLED; offset_idx++)
            for (row_idx = 0; row_idx < DIGIT_IMAGE_HEIGHT_PIXELS; row_idx++)
                for (digit_idx = 0; digit_idx < NUM_DIGIT_IMAGES; digit_idx++)
                    for (pixel_idx = 0; pixel_idx < DIGIT_IMAGE_WIDTH_PIXELS; pixel_idx++)
                    {
                        uint16_t *src = (uint16_t *) digits;
                        uint16_t *dst = (uint16_t *) filldigits + (color_idx * STATUS_PIXELS_FILLED + offset_idx) * DIGIT_IMAGES_SIZE_PIXELS;


                        src += row_idx * DIGIT_IMAGES_ROW_OFFSET_PIXELS + digit_idx * DIGIT_IMAGE_WIDTH_PIXELS + pixel_idx;
                        dst += row_idx * DIGIT_IMAGES_ROW_OFFSET_PIXELS + digit_idx * DIGIT_IMAGE_WIDTH_PIXELS + pixel_idx;

                        if (*src == color_white)
                        {
                            if ((DIGIT_IMAGE_WIDTH_PIXELS - pixel_idx) <= offset_idx)
                                *dst = colors[color_idx][0];    // the darker color
                            else
                                *dst = colors[color_idx][1];    // the lighter color
                        }
                        else
                            *dst = *src;
                    }
    fd = open("/home/pi/homenetworkmon/flash", O_RDONLY);
    assert(fd > 0);

    assert((fstat(fd, &st) == 0) && (st.st_size == FLASH_IMAGES_SIZE_BYTES));

    flash = (char *) mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(flash != MAP_FAILED);
    close(fd);

    memset(lut, -1, sizeof(lut));

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
    lut[' '] = 11;

    struct {
        int x;
        int y;
        int d;
    } config[NUM_PROBE_POINTS] = {
        {0,   0, 0},
        {0,  50, 0},
        {0, 100, 0},
        {0, 150, 0},
        {0, 200, 0},
        {500,   0, 1},
        {500,  50, 1},
        {500, 100, 1},
        {500, 150, 1},
        {500, 200, 1},
    };

    int i;
    for (i = 0; i < NUM_PROBE_POINTS; i++)
    {
        ui[i].x = config[i].x;
        ui[i].y = config[i].y;
        ui[i].d = config[i].d;
    }

    struct timespec last_refresh;

    while (1)
    {
        for (i = 0; i < NUM_PROBE_POINTS; i++)
        {
            unsigned char ii = idx[i] + 255;
            struct timespec now;
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &now) == 0);
            
            int refresh = 0;
            if ((last_refresh.tv_sec > now.tv_sec) || (last_refresh.tv_sec < now.tv_sec - 10))
            {
                last_refresh = now;
                refresh = 1;
            }

            long deltal = (now.tv_nsec - t[i][ii].tv_nsec) / 100000L;
            long deltah = now.tv_sec - t[i][ii].tv_sec;
            assert(deltah >= 0);
            while (deltah)
            {
                deltah--;
                deltal += 10000;
            }

            update(ui + i, deltal, refresh);
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
    for (i = 0; i < NUM_PROBE_POINTS; i++)
    {
        idx[i] = 0;
        assert(clock_gettime(CLOCK_MONOTONIC_RAW, &t[i][255]) == 0);
    }

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
                if ((i >= 0) && (i < NUM_PROBE_POINTS))
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
            if ((i >= 0) && (i < NUM_PROBE_POINTS))
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
