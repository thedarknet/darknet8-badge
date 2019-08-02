#include <atmel_start.h>
#include <atomic.h>

#define NONE 0xff

typedef void (*LEDFUN)(const bool) ;

uint8_t I2C_0_slave_address=NONE;
uint8_t I2C_0_register_address=NONE;
//uint8_t I2C_0_cmd = NONE;


void I2C_0_address_handler()
{
	I2C_0_slave_address = I2C_0_read();
	I2C_0_send_ack(); // or send_nack() if we don't want to ack the address
	//I2C_0_num_addresses++;
}

void I2C_0_read_handler()
{ // Master read operation
	I2C_0_write(0x0c);
	//I2C_0_num_reads++;
}

void I2C_0_write_handler()
{ // Master write handler
	if(I2C_0_register_address==NONE) {
		I2C_0_register_address = I2C_0_read();
		I2C_0_send_ack(); // or send_nack() if we don't want to receive more data
	//} else if(I2C_0_cmd==NONE) {
		//I2C_0_cmd = I2C_0_read();
		//I2C_0_send_ack(); // or send_nack() if we don't want to receive more data
	} else {
		I2C_0_read();
		I2C_0_send_nack(); // or send_nack() if we don't want to receive more data
	}
	//I2C_0_num_writes++;
}

void I2C_0_stop_handler()
{
	//I2C_0_num_stops++;
}

void I2C_0_error_handler()
{
	while (1)
	;
}

uint8_t setupSlave(void)
{

	I2C_0_enable();
	I2C_0_set_read_callback(I2C_0_read_handler);
	I2C_0_set_write_callback(I2C_0_write_handler);
	I2C_0_set_address_callback(I2C_0_address_handler);
	I2C_0_set_stop_callback(I2C_0_stop_handler);
	I2C_0_set_collision_callback(I2C_0_error_handler);
	I2C_0_set_bus_error_callback(I2C_0_error_handler);
	I2C_0_open();

	return 1;
}
#define TOTAL_LEDS 24
static LEDFUN LedFunctions[TOTAL_LEDS];

void setupLeds() {
	LedFunctions[0] = &LED1_set_level;
	LedFunctions[1] = &LED2_set_level;
	LedFunctions[2] = &LED3_set_level;
	LedFunctions[3] = &LED4_set_level;
	LedFunctions[4] = &LED5_set_level;
	LedFunctions[5] = &LED6_set_level;
	LedFunctions[6] = &LED7_set_level;
	LedFunctions[7] = &LED8_set_level;
	LedFunctions[8] = &LED9_set_level;
	LedFunctions[9] = &LED10_set_level;
	LedFunctions[10] = &LED11_set_level;
	LedFunctions[11] = &LED12_set_level;
	LedFunctions[12] = &LED13_set_level;
	LedFunctions[13] = &LED14_set_level;
	LedFunctions[14] = &LED15_set_level;
	LedFunctions[15] = &LED16_set_level;
	LedFunctions[16] = &LED17_set_level;
	LedFunctions[17] = &LED18_set_level;
	LedFunctions[18] = &LED19_set_level;
	LedFunctions[19] = &LED20_set_level;
	LedFunctions[20] = &LED21_set_level;
	LedFunctions[21] = &LED22_set_level;
	LedFunctions[22] = &LED23_set_level;
	LedFunctions[23] = &LED24_set_level;
	
}

typedef struct DANCEFUN {
	timercallback_ptr_t light;
	absolutetime_t TimeOut;
	uint32_t ExtraData;
	void *next;
} LedDanceFun;

static timer_struct_t LedTimer;

absolutetime_t queueNext(LedDanceFun *data) {
	if(data) {
		LedTimer.callback_ptr = data->light;
		LedTimer.payload = data;
		return data->TimeOut;
	} 
	return 0; //stop timer
}

absolutetime_t turnAll(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		(*LedFunctions[i])(data->ExtraData);
	}
	return queueNext(data->next);
}

absolutetime_t turnOnHalf(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		if((i&0x1)) {
			(*LedFunctions[i])(data->ExtraData);
		} else {
			(*LedFunctions[i])(!data->ExtraData);
		}
	}
	return queueNext(data->next);
}

absolutetime_t turnOnEveryN(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		if((i%data->ExtraData)==0) {
			(*LedFunctions[i])(true);
			} else {
			(*LedFunctions[i])(false);
		}
	}
	return queueNext(data->next);
}

LedDanceFun Initial5 = {&turnAll     ,30000,0,0};
LedDanceFun Initial4 = {&turnOnEveryN,30000,8,&Initial5};
LedDanceFun Initial3 = {&turnOnEveryN,30000,4,&Initial4};
LedDanceFun Initial2 = {&turnOnHalf  ,30000,1,&Initial3};
LedDanceFun Initial1 = {&turnOnHalf  ,30000,1,&Initial2};
LedDanceFun Initial0 = {&turnOnHalf  ,30000,0,&Initial1};
LedDanceFun Initial  = {&turnAll     ,30000,1,&Initial0};


int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	setupLeds();
	setupSlave();
	
	
	ENABLE_INTERRUPTS();
	LedTimer.callback_ptr = turnAll;
	LedTimer.payload = &Initial;
	TIMER_0_timeout_create(&LedTimer, 1000);
		
	/* Replace with your application code */
	while (1) {
		if(NONE!=I2C_0_register_address) {
			I2C_0_register_address=NONE;
			switch(I2C_0_register_address) {
				case 1:
					//turnOnHalf();
					break;
				case 2:
					//turnOnQuarter();
					break;
				case 3:
					break;
			}
		}
		TIMER_0_timeout_call_next_callback();
	}
}
