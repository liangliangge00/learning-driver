	...
	...
	...

	char buf;
	fd = open("/dev/ttyS1", O_RDWR | O_NOBLOCK);
	...

	while (read(fd, &buf, 1) == 1)
		continue;
	
	printf("%c\n", buf);
	return 0;

