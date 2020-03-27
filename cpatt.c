#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>

#define fatal(fmt, ...) do { \
	fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while (0)	

typedef uint32_t u_t;
static const u_t sep = 0xefefefef; /* must be repetitive; */
#define UU PRIx32

typedef struct map {
	int fd;
	uint8_t *ptr;
	size_t size;
} map_t;

int
map_file(map_t *map, const char *path, bool rw)
{
	struct stat stat;

	if ((map->fd = open(path, rw ? O_RDWR : O_RDONLY)) < 0)
		fatal("open(): %s", strerror(errno));
	if ((fstat(map->fd, &stat)) < 0)
		fatal("fstat(): %s", strerror(errno));
	if (stat.st_size >= 0x80000000)
		fatal("too large file");
	if ((map->ptr = mmap(NULL, stat.st_size, PROT_READ | (rw? PROT_WRITE: 0),
						 MAP_SHARED, map->fd, 0)) == MAP_FAILED)
		fatal("mmap(): %s", strerror(errno));
	map->size = stat.st_size;
	return 0;
}

int test(map_t *map)
{
	u_t val = 0, pos = 0;
	int c = 0, csep = 0;
	int seg_stat = 0; /* 1=misaligned, 2=corrupt */
	int off = 0;
	for (uint8_t *el = (void *)map->ptr; el < &map->ptr[map->size]; el++)
	{
		pos = el - map->ptr - 3;
		val += *el << (c * 8); /* little-endian */
		c++;

		if (*el == (sep & 0xff))
			csep++;
		else
			csep = 0;

		if (csep == 4) {
			c = 0, val = 0, csep = 0;
			if (seg_stat == 2)
				printf("@%" UU ": corrupt segment ends\n", pos);
			seg_stat = 0;
		}

		if (c == 4) {
			u_t expected = off + pos;
			if (val != expected) {
				if (seg_stat == 0 && val < map->size) {
					off += (int)val - (int)expected;
					seg_stat = 1;
					printf("@%" UU ": unexpected value %" UU "; "
						   "assuming misalignement\n", pos, val);
				} else if (seg_stat != 2) {
					seg_stat = 2;
					printf("@%" UU ": unexpected value %" UU "; "
						   "assuming corruption\n", pos, val);
				}
			} 
			c = 0, val = 0;
		}
	}
	return 0;
}

int write(map_t *map)
{
	for (u_t *el = (u_t *)map->ptr, val = 0;
		 el < (u_t *)&map->ptr[(map->size / sizeof(u_t)) * sizeof(u_t)];
		 el++, val += sizeof(u_t))
	{
		*el = val;
		if ((el - (u_t *)map->ptr) % 2 == 0)
			*el = sep;
	}
	return 0;
}



int
main(int argc, char **argv)
{
	map_t map;
	bool rw = (argv[1][0] == 'w');
	map_file(&map, argv[2], rw);

	return (rw? write: test)(&map);
}
