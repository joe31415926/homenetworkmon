#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>
     #include <time.h>

// ffmpeg -y -i congrats.jpg -vcodec rawvideo -f rawvideo -s 1280x720 -pix_fmt rgb565 out
// ffmpeg -y -i digits.png -vcodec rawvideo -f rawvideo -pix_fmt rgb565 digits

int main()
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
    
    fd = open("out", O_RDONLY);
    assert(fd > 0);

    struct stat buf;
    assert((fstat(fd, &buf) == 0) && (buf.st_size == 1280 * 720 * 2));
    
    char *fbmm = (char *) mmap(0, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(fbmm != MAP_FAILED);
    close(fd);

    fd = open("digits", O_RDONLY);
    assert(fd > 0);

    struct stat buf2;
    assert((fstat(fd, &buf2) == 0) && (buf2.st_size == 11 * 32 * 48 * 2));
    
    char *digits = (char *) mmap(0, buf2.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(digits != MAP_FAILED);
    close(fd);


    memcpy(framebuffer, fbmm, 1280 * 720 * 2);
    
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
    
    while (1)
    {
        struct timespec tp;
        assert(clock_gettime(CLOCK_MONOTONIC_RAW, &tp) == 0);
        
        char buf[20];
        sprintf(buf, "%09d.%09d", tp.tv_sec, tp.tv_nsec);
        
        for (i = 0; i < 19; i++)
        {
            int offset = lut[buf[i]];
            assert(offset >= 0);
            
            int j;
            for (j = 0; j < 48; j++)
                memcpy(framebuffer + 1280 * 2 * j + 32 * 2 * i, digits + 11 * 32 * 2 * j + 32 * 2 * offset, 32 * 2);
        }
    }
    
    munmap(digits, buf2.st_size);
    munmap(fbmm, buf.st_size);
    munmap(framebuffer, finfo.smem_len);
}
