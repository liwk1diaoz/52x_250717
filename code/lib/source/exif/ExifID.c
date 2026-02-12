#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"

#include "exif/Exif.h"
#include <kwrap/verinfo.h>

ID SEMID_EXIF[EXIF_HDL_MAX_NUM] = {0};

VOS_MODULE_VERSION(Exif, 1, 00, 002, 00)

void EXIF_InstallID(void)
{
	UINT32 i;
	static char sem_name[EXIF_HDL_MAX_NUM][8] = {
		"EXIFID0",
		"EXIFID1",
		"EXIFID2",
		"EXIFID3",
	};

	for (i = 0; i < EXIF_HDL_MAX_NUM; i++) {
		vos_sem_create(&SEMID_EXIF[i], 1, sem_name[i]);
	}
}

void EXIF_UninstallID(void)
{
	UINT32 i;

	for (i = 0; i < EXIF_HDL_MAX_NUM; i++) {
		vos_sem_destroy(SEMID_EXIF[i]);
		SEMID_EXIF[i] = 0;
	}
}

