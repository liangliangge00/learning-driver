	...
	...
	...

	char buf;
	fd = open("/dev/ttyS1", O_RDWR);
	...

	res = read(fd, &buf, 1);
	if (res == 1)
	printf("%c\n", buf);
	return 0;