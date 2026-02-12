// ConsoleApplication1.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <string.h>
#include <libfdt.h>
#include "compiler.h"

int main(void)
{
	const char dtb_path[] = "/etc/application.dtb";

	FILE *fp;
	int i, len;
	unsigned char *buf;
	const void *nodep;  /* property node pointer */
	struct fdt_header fdt_hdr = {0};

	// load dtb to memory
	fp = fopen(dtb_path, "rb");
	if (fp == NULL) {
		printf("failed to open %s\n", dtb_path);
		return -1;
	}

	fread(&fdt_hdr, sizeof(fdt_hdr), 1, fp);
	fseek(fp, 0, SEEK_SET);

	int fdt_size = fdt_totalsize(&fdt_hdr);
	buf = (unsigned char *)malloc(fdt_totalsize(&fdt_hdr));

	fread(buf, fdt_size, 1, fp);
	fclose(fp);

	// read /sensor@1/cfg
	int nodeoffset = fdt_path_offset(buf, "/sensor@1");
	if (nodeoffset < 0) {
		free(buf);
		return -1;
	}

	nodep = fdt_getprop(buf, nodeoffset, "cfg", &len);
	if (len == 0) {
		free(buf);
		return -1;
	}
	const char *p_cfg = (const char *)nodep;
	printf("cfg: ");
	for (i = 0; i < len; i++) {
		printf("%s ", p_cfg);
		i += strlen(p_cfg);
		p_cfg += strlen(p_cfg) + 1; // +1 for skip \0
	}
	printf("\n");

	// read /sensor@1/sie
	nodep = fdt_getprop(buf, nodeoffset, "sie", &len);
	if (len == 0) {
		free(buf);
		return -1;
	}
	int n = len / sizeof(unsigned int);
	unsigned int *p_sie = (unsigned int *)nodep;
	printf("sie: ");
	for (i = 0; i < n; i++) {
		printf("%u ", be32_to_cpu(p_sie[i]));
	}
	printf("\n");

	free(buf);
	return 0;
}