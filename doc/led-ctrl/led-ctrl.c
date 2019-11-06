#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define device_path "/sys/class/leds/led-red/brightness"

int main(int argc, char *argv[])
{
	int i,fd,ret;
	
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d] = %s\n", i, argv[i]);
	
	fd = open(device_path, O_RDWR);
	if (fd == -1) {
		printf("open device file failed\n");
		return fd;
	}

	if (0 == strcmp(argv[i-1], "on")) {
		ret = write(fd, "1", 1);
		printf("led open successfully\n");
	}
	else if (0 == strcmp(argv[i-1], "off")) {
		write(fd, "0", 1);
		printf("led close successfully\n");
	}
	else 
		printf("not define led state\n");
	
	close(fd);
	return 0;
}
