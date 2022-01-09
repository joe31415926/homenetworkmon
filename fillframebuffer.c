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

void placeimage(uint16_t *fbout, int xoff, int yoff, const char *filename, int width, int height)
{
    fbout += xoff + yoff * 1280;
    
    int fd = open(filename, O_RDONLY);
    assert(fd > 0);

    char *fbmm = (char *) mmap(0, width * height * 4, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(fbmm != MAP_FAILED);
    close(fd);

    uint8_t *fbin = (uint8_t *) fbmm;
    int y, x;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int red = *fbin++;
            int green = *fbin++;
            int blue = *fbin++;
            int alpha = *fbin++;
            
            red >>= 3;  // 5 bits
            green >>= 2; // 6 bits
            blue >>= 3; // 5 bits
            
            *fbout++ = (red << 11) | (green << 5) | blue; 
        }
        
        fbout += 1280 - width;
    }
    munmap(fbmm, width * height * 4);
}

void clearcolumn(uint16_t *fbout, int a, int b)
{
    fbout += a;
    int i;
    for (i = 0; i < 720; i++)
    {
        memset(fbout, 0, 2 * (b - a));
        fbout += 1280;
    }
}

void clearrow(uint16_t *fbout, int a, int b)
{
    fbout += a * 1280;
    memset(fbout, 0, 2 * 1280 * (b - a));
}

int main()
{
    int fd = open("/dev/fb0", O_RDWR);
    assert(fd > 0);
    
    struct fb_fix_screeninfo finfo;
    assert(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == 0);
    assert(finfo.smem_len == 1843200);
    
    struct fb_var_screeninfo vinfo;
    assert(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0);
    assert(vinfo.xres == 1280);
    assert(vinfo.yres == 720);
    assert(vinfo.bits_per_pixel == 16);
    
    char *framebuffer = (char *) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(framebuffer != MAP_FAILED);
    close(fd);
    
    clearrow((uint16_t *) framebuffer, 0, 55);
    clearrow((uint16_t *) framebuffer, 355, 365);
    clearrow((uint16_t *) framebuffer, 665, 720);
    
    clearcolumn((uint16_t *) framebuffer, 0, 135);
    clearcolumn((uint16_t *) framebuffer, 635, 645);
    clearcolumn((uint16_t *) framebuffer, 1145, 1280);
    
    placeimage((uint16_t *) framebuffer, 135, 55, "/home/pi/ramdisk/log0.rgba", 500, 300);
    placeimage((uint16_t *) framebuffer, 645, 55, "/home/pi/ramdisk/log1.rgba", 500, 300);
    placeimage((uint16_t *) framebuffer, 135, 365, "/home/pi/ramdisk/log2.rgba", 500, 300);
    placeimage((uint16_t *) framebuffer, 645, 365, "/home/pi/ramdisk/log3.rgba", 500, 300);
//    placeimage((uint16_t *) framebuffer, 10, 10, "/home/pi/ramdisk/log4.rgba", 500, 300);
    
    munmap(framebuffer, finfo.smem_len);
}
