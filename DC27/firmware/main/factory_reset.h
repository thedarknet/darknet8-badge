#ifndef _DN8_FACTORY_RESET_
#define _DN8_FACTORY_RESET_
#include "esp_system.h"
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "esp_ota_ops.h"
#include "string.h"

const char* FRST_LOGTAG = "FACTORY_RESET";

static void do_factory_reset(void)
{
	uint32_t my_nvs_handle = 0;
	nvs_open("nvs", NVS_READWRITE, &my_nvs_handle);

	const esp_partition_t* esp_part = NULL;
	esp_partition_iterator_t itr;

	itr = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
	if (itr == NULL)
	{   
		ESP_LOGE(FRST_LOGTAG, "Failed to find factory partition");
	}

	esp_part = esp_partition_get(itr);
	esp_ota_set_boot_partition(esp_part);

	nvs_set_i32(my_nvs_handle, "attempted_boot", 0);
	nvs_set_i32(my_nvs_handle, "boot_successful", 0);
	nvs_commit(my_nvs_handle);

	esp_restart();
}


#endif
