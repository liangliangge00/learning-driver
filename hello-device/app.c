#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int fd;

	fd = open("/dev/hello-device", O_RDWR);
	if (fd < 0) {
		printf("open fail\n");
		exit(-1);
	}

	printf("open success\n");
	close(fd);

	return 0;
}
