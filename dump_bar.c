/*
 *  Copyright (C) 2016, Jonathan Derrick (jonathan.derrick@intel.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Requires nopat kernel argument and CONFIG_STRICT_DEVMEM=n */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>


#define PRINT_ERROR(str) do { fprintf(stderr, str "%s\n", strerror(errno)); exit(1); } while (0)
int main(int argc, char **argv) {
	int sfd, dfd;
	unsigned long long ps, i, j, k;
	unsigned long long offset, span;
	uint64_t *src_mmap, *dest_mmap;
	const char *out;

	ps = sysconf(_SC_PAGE_SIZE);
	if (ps < 0)
		PRINT_ERROR("Couldn't get page size");

	//FIXME: check return
	if (argc < 3)
		PRINT_ERROR("./dump_bar <offset> <span> <output file>");

	offset = strtoull(argv[1], 0, 0);
	span = strtoull(argv[2], 0, 0);
	out = argv[3];
	fprintf(stderr, "Page Size %#x Offset %#llx Span %#llx\n", ps, offset, span);

	if ((sfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
		PRINT_ERROR("Failed to open src");
	if ((dfd = open(out, O_RDWR | O_TRUNC | O_CREAT, 0666)) < 0)
		PRINT_ERROR("Failed to open dest");
	ftruncate(dfd, span);

	for (i = 0; i < span; i += ps) {
		src_mmap = mmap(0, ps, PROT_READ, MAP_SHARED, sfd, offset + i);
		if (src_mmap == (void *) -1)
			PRINT_ERROR("Failed to mmap src");

		dest_mmap = mmap(0, ps, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, i);
		if (dest_mmap == (void *) -1)
			PRINT_ERROR("Failed to mmap dest");

		memcpy(dest_mmap, src_mmap, ps);

		if (munmap(dest_mmap, ps) == -1)
			PRINT_ERROR("Failed to munmap dest");
		if (munmap(src_mmap, ps) == -1)
			PRINT_ERROR("Failed to munmap src");
	}
	close(dfd);
	close(sfd);
}
