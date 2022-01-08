#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork(), sleep(), read(), close()
#include <assert.h>
#include <stdint.h>
#include <fcntl.h>      // open()
#include <time.h>       // time()
#include <poll.h>       // poll()
#include <sys/wait.h>   // wait()
#include <sys/inotify.h>

struct record {
    uint32_t timestamp;
    uint16_t destination;
    uint16_t durationms;
} record;

struct {
    struct record *history;
    int len;
    int size;
    int dirty;
} destinations[5];

void updatefiles()
{
    int i;
    for (i = 0; i < sizeof(destinations) / sizeof(destinations[0]); i++)
        if (destinations[i].dirty)
        {
            FILE *f = fopen("/home/pi/ramdisk/temp.txt", "w");
            assert(f);
            int j;
            for (j = 0; j < destinations[i].len; j++)
                fprintf(f, "%d %d %d\n", destinations[i].history[j].timestamp, destinations[i].history[j].destination, destinations[i].history[j].durationms);
            fclose(f);
            char filename[100];
            sprintf(filename, "/home/pi/ramdisk/log%d.txt", i);
            assert(rename("/home/pi/ramdisk/temp.txt", filename) == 0);
            destinations[i].dirty = 0;
        }
}

void readbytesin(int fd_pinglog, char **destination_buffer, ssize_t *bytes_to_read)
{
    while (1)
    {
        ssize_t bytes_just_read = read(fd_pinglog, *destination_buffer, *bytes_to_read);
        assert(bytes_just_read >= 0);
        if (bytes_just_read == 0)
            return;

        (*destination_buffer) += bytes_just_read;
        (*bytes_to_read) -= bytes_just_read;
        
        if (*bytes_to_read == 0)
        {
            (*destination_buffer) = (char *) &record;
            (*bytes_to_read) = sizeof(record);
            
            assert(record.destination < sizeof(destinations) / sizeof(destinations[0]));
            
            if (destinations[record.destination].len == destinations[record.destination].size)
            {
                destinations[record.destination].size *= 2;
                destinations[record.destination].history = realloc(destinations[record.destination].history, destinations[record.destination].size * sizeof(struct record));
                assert(destinations[record.destination].history != NULL);
            }
            destinations[record.destination].history[destinations[record.destination].len].timestamp = record.timestamp;
            destinations[record.destination].history[destinations[record.destination].len].destination = record.destination;
            destinations[record.destination].history[destinations[record.destination].len].durationms = record.durationms;
            destinations[record.destination].len++;
            destinations[record.destination].dirty = 1;
        }
    }
}

void child()
{
    int i;
    for (i = 0; i < sizeof(destinations) / sizeof(destinations[0]); i++)
    {
        destinations[i].len = 0;
        destinations[i].size = 10;
        destinations[i].dirty = 0;
        destinations[i].history = (struct record *) malloc(destinations[i].size * sizeof(struct record));
        assert(destinations[i].history != NULL);
    }
    
    FILE *logfile = fopen("/home/pi/ramdisk/parseping.log", "a");
    if (logfile)
    {
        fprintf(logfile, "restart %d\n", (int) time(NULL));
        fclose(logfile);
    }

    int fd_inotify = inotify_init();
    assert(fd_inotify > 0);
    
    assert(inotify_add_watch(fd_inotify, "/home/pi/ping.log", IN_MODIFY) > 0);

    int fd_pinglog = open("/home/pi/ping.log", O_RDONLY);
    assert(fd_pinglog > 0);
    
    char *destination_buffer = (char *) &record;
    ssize_t bytes_to_read = sizeof(record);
    
    // flush out the file until EOF
    readbytesin(fd_pinglog, &destination_buffer, &bytes_to_read);
    
    while (1)
    {
        struct inotify_event evt;
        read(fd_inotify, &evt, sizeof(evt));
        
        readbytesin(fd_pinglog, &destination_buffer, &bytes_to_read);
        updatefiles();
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
