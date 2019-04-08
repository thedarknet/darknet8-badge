#ifndef DC27_BLUETOOTH
#define DC27_BLUETOOTH

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <libesp/Task.h>

enum
{
	DN8_IDX_SVC,

	DN8_IDX_NB,
};

class BluetoothTask : public Task {
public:
	static const char *LOGTAG;

	BluetoothTask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~BluetoothTask();
};

#endif // DC27_BLUETOOTH
