#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libfdt.h>

#define CFG_FDT_PATH "/sys/firmware/fdt"

static void *m_pfdt = NULL;

#define SHMEM_PATH "/nvt_memory_cfg"

const void *fdt_get_base(void)
{
	if (m_pfdt != NULL) {
		return m_pfdt;
	}

	struct stat st;
	if (stat(CFG_FDT_PATH, &st) != 0) {
		printf("failed to get %s, err = %d \n", CFG_FDT_PATH, errno);
		return NULL;
	}

	int fd = open(CFG_FDT_PATH, O_RDONLY);
	if (fd < 0) {
		printf("failed to open %s, err = %d \n", CFG_FDT_PATH, errno);
		return NULL;
	}

	m_pfdt = malloc(st.st_size);
	int size = st.st_size;
	unsigned char *pfdt = (unsigned char *)m_pfdt;
	while (size > 0) {
		int read_size = (size > 4096) ? 4096 : size;
		read(fd, pfdt, read_size);
		size -= read_size;
		pfdt += read_size;

	}
	close(fd);

	int er = fdt_check_header(m_pfdt);
	if (er != 0) {
		printf("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)m_pfdt, er);
		free(m_pfdt);
		m_pfdt = NULL;
		return m_pfdt;
	}

	if (st.st_size != fdt_totalsize(m_pfdt)) {
		printf("fdt size not matched. %d(st.st_size) != %d(fdt_totalsize()) \n",
			st.st_size,
			fdt_totalsize(m_pfdt));
		free(m_pfdt);
		m_pfdt = NULL;
		return m_pfdt;
	}

	unsigned char *p_fdt = (unsigned char *)m_pfdt;

	if (p_fdt == NULL) {
		printf("p_fdt is NULL. \n");
		return -1;
	}

	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */

	// read SHMEM_PATH
	nodeoffset = fdt_path_offset(p_fdt, SHMEM_PATH);
	if (nodeoffset < 0) {
		printf("failed to offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	} else {
		printf("offset for  %s = %d \n", SHMEM_PATH, nodeoffset);
	}

	return m_pfdt;
}