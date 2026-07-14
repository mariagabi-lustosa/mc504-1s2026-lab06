#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "lkcamp_ioctl.h"

int main()
{
    int fd;
    int ret;
    unsigned char key;
    const char *msg = "LAB06 DE SO";
    char out[64];
    int n;

    fd = open("/dev/cipher", O_RDWR);
    if (fd < 0) {
        printf("error opening /dev/cipher\n");
        return 1;
    }

    /* Set the correct key and write the message */
    key = 5;
    ret = ioctl(fd, LKCAMP_SET_KEY, &key);
    if (ret == -1) {
        printf("error on ioctl SET_KEY\n");
        return 1;
    }
    printf("key set to %d\n", key);

    ret = write(fd, msg, 11);
    if (ret != 11) {
        printf("error writing to device, wrote %d bytes\n", ret);
        return 1;
    }
    printf("wrote: %s\n", msg);

    /* Reset position before reading back what we just wrote */
    lseek(fd, 0, SEEK_SET);

    n = read(fd, out, sizeof(out) - 1);
    if (n < 0) {
        printf("error reading from device\n");
        return 1;
    }
    out[n] = '\0';
    printf("read with correct key: %s\n", out);

    /* Now set a different (wrong) key and read the same stored data again */
    key = 9;
    ret = ioctl(fd, LKCAMP_SET_KEY, &key);
    if (ret == -1) {
        printf("error on ioctl SET_KEY\n");
        return 1;
    }
    printf("key set to %d\n", key);

    lseek(fd, 0, SEEK_SET);

    n = read(fd, out, sizeof(out) - 1);
    if (n < 0) {
        printf("error reading from device\n");
        return 1;
    }
    out[n] = '\0';
    printf("read with wrong key: %s\n", out);

    close(fd);

    return 0;
}
