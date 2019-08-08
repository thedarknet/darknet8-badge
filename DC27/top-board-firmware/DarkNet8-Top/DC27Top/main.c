#include <atmel_start.h>
#include <atomic.h>
#include <stdlib.h>

#define NONE 0xff

typedef void (*LEDFUN)(const bool) ;

uint8_t I2C_0_slave_address=NONE;
uint8_t I2C_0_register_address=NONE;
uint8_t I2C_0_cmd = NONE;
//uint32_t LedCache =0;
//volatile uint8_t dutyPct = 100;


void I2C_0_address_handler() {
	I2C_0_slave_address = I2C_0_read();
	I2C_0_send_ack(); // or send_nack() if we don't want to ack the address
	//I2C_0_num_addresses++;
}

void I2C_0_read_handler() { // Master read operation
	I2C_0_write(0x0c);
	//I2C_0_num_reads++;
}

void I2C_0_write_handler() { // Master write handler
	if(I2C_0_register_address==NONE) {
		I2C_0_register_address = I2C_0_read();
		I2C_0_send_ack(); // or send_nack() if we don't want to receive more data
	} else if(I2C_0_cmd==NONE) {
		I2C_0_cmd = I2C_0_read();
		I2C_0_send_nack(); // or send_nack() if we don't want to receive more data
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
static uint8_t LEDS[TOTAL_LEDS];

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

absolutetime_t queueNext(LedDanceFun *data);

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
			LEDS[i]=data->ExtraData;
		} else {
			LEDS[i]=data->ExtraData;
		}
	}
	return queueNext(data->next);
}

absolutetime_t turnOnEveryN(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		if((i%data->ExtraData)==0) {
			LEDS[i]=255;
			} else {
			LEDS[i]=0;
		}
	}
	return queueNext(data->next);
}

absolutetime_t turnOnOffRandom(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	static uint16_t count = 0;
	count++;
	for(int i=0;i<data->ExtraData;++i)
	{
		LEDS[rand() % TOTAL_LEDS]=255;
		LEDS[rand() % TOTAL_LEDS]=0;
	}

	if(count>1000) {
		count=0;
		return queueNext(data->next);
	}
	return queueNext(data);
}

absolutetime_t turnOnRandom(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	static uint16_t state = 0;
	
	LEDS[rand() % TOTAL_LEDS]=data->ExtraData;
	
    state = 1;
	for(int i=0;i<TOTAL_LEDS;++i) {
      if(LEDS[i] != data->ExtraData)
      {
        state = 0; 
      }
    }
	if(state) {
		return queueNext(data->next);
	}
	return queueNext(data);
}


absolutetime_t danceVertical(void *d) {
	static int8_t danc1_value = 1;
	static uint8_t count = 0;
	LedDanceFun *data = (LedDanceFun *)d;
	switch(data->ExtraData) {
		case 0:
			data->TimeOut = 5000;
			danc1_value = 1;
			data->ExtraData+=danc1_value;
			count++;
			break;
		case 1:
			LEDS[2]=(danc1_value>0?255:0);
			LEDS[5]=(danc1_value>0?255:0);
			LEDS[8]=(danc1_value>0?255:0);
			LEDS[11]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
			break;
		case 2:
			LEDS[1]=(danc1_value>0?255:0);
			LEDS[4]=(danc1_value>0?255:0);
			LEDS[7]=(danc1_value>0?255:0);
			LEDS[10]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
		break;
		case 3:
			LEDS[0]=(danc1_value>0?255:0);
			LEDS[3]=(danc1_value>0?255:0);
			LEDS[6]=(danc1_value>0?255:0);
			LEDS[9]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
		break;
		case 4:
			LEDS[14]=(danc1_value>0?255:0);
			LEDS[17]=(danc1_value>0?255:0);
			LEDS[20]=(danc1_value>0?255:0);
			LEDS[23]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
		break;
		case 5:
			LEDS[13]=(danc1_value>0?255:0);
			LEDS[16]=(danc1_value>0?255:0);
			LEDS[19]=(danc1_value>0?255:0);
			LEDS[22]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
		break;
		case 6:
			LEDS[12]=(danc1_value>0?255:0);
			LEDS[15]=(danc1_value>0?255:0);
			LEDS[18]=(danc1_value>0?255:0);
			LEDS[21]=(danc1_value>0?255:0);
			data->TimeOut = 10000;
			data->ExtraData+=danc1_value;
		break;
		case 7:
			data->TimeOut = 5000;
			danc1_value = -1;
			data->ExtraData+=danc1_value;
		break;
	}
	if(count>5) {
		count=0;
		danc1_value = 1;
		return queueNext(data->next);
	}
	return queueNext(data);
}

absolutetime_t danceVerticalCylon(void *d) {
	static int8_t danc1_value = 1;
	static uint8_t count = 0;
	LedDanceFun *data = (LedDanceFun *)d;
	switch(data->ExtraData) {
		case 0:
		data->TimeOut = 5000;
		danc1_value = 1;
		data->ExtraData+=danc1_value;
		count++;
		break;
		case 1:
		LEDS[2]=255;
		LEDS[5]=255;
		LEDS[8]=255;
		LEDS[11]=255;

		LEDS[1]=0;
		LEDS[4]=0;
		LEDS[7]=0;
		LEDS[10]=0;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 2:
		LEDS[2]=0;
		LEDS[5]=0;
		LEDS[8]=0;
		LEDS[11]=0;

		LEDS[1]=255;
		LEDS[4]=255;
		LEDS[7]=255;
		LEDS[10]=255;

		LEDS[0]=0;
		LEDS[3]=0;
		LEDS[6]=0;
		LEDS[9]=0;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 3:
		LEDS[1]=0;
		LEDS[4]=0;
		LEDS[7]=0;
		LEDS[10]=0;

		LEDS[0]=255;
		LEDS[3]=255;
		LEDS[6]=255;
		LEDS[9]=255;

		LEDS[14]=0;
		LEDS[17]=0;
		LEDS[20]=0;
		LEDS[23]=0;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 4:
		LEDS[0]=0;
		LEDS[3]=0;
		LEDS[6]=0;
		LEDS[9]=0;

		LEDS[14]=255;
		LEDS[17]=255;
		LEDS[20]=255;
		LEDS[23]=255;

		LEDS[13]=0;
		LEDS[16]=0;
		LEDS[19]=0;
		LEDS[22]=0;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 5:
		LEDS[14]=0;
		LEDS[17]=0;
		LEDS[20]=0;
		LEDS[23]=0;

		LEDS[13]=255;
		LEDS[16]=255;
		LEDS[19]=255;
		LEDS[22]=255;

		LEDS[12]=0;
		LEDS[15]=0;
		LEDS[18]=0;
		LEDS[21]=0;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 6:
		LEDS[13]=0;
		LEDS[16]=0;
		LEDS[19]=0;
		LEDS[22]=0;

		LEDS[12]=255;
		LEDS[15]=255;
		LEDS[18]=255;
		LEDS[21]=255;
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 7:
		data->TimeOut = 5000;
		danc1_value = -1;
		data->ExtraData+=danc1_value;
		break;
	}
	if(count>5) {
		count=0;
		danc1_value = 1;
		return queueNext(data->next);
	}
	return queueNext(data);
}

absolutetime_t danceHorizonal(void *d) {
	static int8_t danc1_value = 1;
	static uint8_t count = 0;
	LedDanceFun *data = (LedDanceFun *)d;
	switch(data->ExtraData) {
		case 0:
		data->TimeOut = 5000;
		danc1_value = 1;
		data->ExtraData+=danc1_value;
		count++;
		break;
		case 1:
		LEDS[0]=(danc1_value>0?255:0);
		LEDS[1]=(danc1_value>0?255:0);
		LEDS[2]=(danc1_value>0?255:0);
		LEDS[23]=(danc1_value>0?255:0);
		LEDS[22]=(danc1_value>0?255:0);
		LEDS[21]=(danc1_value>0?255:0);
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 2:
		LEDS[5]=(danc1_value>0?255:0);
		LEDS[4]=(danc1_value>0?255:0);
		LEDS[3]=(danc1_value>0?255:0);
		LEDS[20]=(danc1_value>0?255:0);
		LEDS[19]=(danc1_value>0?255:0);
		LEDS[18]=(danc1_value>0?255:0);
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 3:
		LEDS[8]=(danc1_value>0?255:0);
		LEDS[7]=(danc1_value>0?255:0);
		LEDS[6]=(danc1_value>0?255:0);
		LEDS[17]=(danc1_value>0?255:0);
		LEDS[16]=(danc1_value>0?255:0);
		LEDS[15]=(danc1_value>0?255:0);
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 4:
		LEDS[11]=(danc1_value>0?255:0);
		LEDS[10]=(danc1_value>0?255:0);
		LEDS[9]=(danc1_value>0?255:0);
		LEDS[14]=(danc1_value>0?255:0);
		LEDS[13]=(danc1_value>0?255:0);
		LEDS[12]=(danc1_value>0?255:0);
		data->TimeOut = 10000;
		data->ExtraData+=danc1_value;
		break;
		case 5:
		data->TimeOut = 5000;
		danc1_value = -1;
		data->ExtraData+=danc1_value;
		break;
	}
	if(count>5) {
		count=0;
		danc1_value = 1;
		return queueNext(data->next);
	}
	return queueNext(data);
}

absolutetime_t danceRain(void *d) {
	static uint16_t count = 0;
	LedDanceFun *data = (LedDanceFun *)d;

    count++;

	LEDS[0]=LEDS[5];
	LEDS[1]=LEDS[4];
	LEDS[2]=LEDS[3];
	LEDS[23]=LEDS[20];
	LEDS[22]=LEDS[19];
	LEDS[21]=LEDS[18];

	LEDS[5]=LEDS[8];
	LEDS[4]=LEDS[7];
	LEDS[3]=LEDS[6];
	LEDS[20]=LEDS[17];
	LEDS[19]=LEDS[16];
	LEDS[18]=LEDS[15];

	LEDS[8]=LEDS[11];
	LEDS[7]=LEDS[10];
	LEDS[6]=LEDS[9];
	LEDS[17]=LEDS[14];
	LEDS[16]=LEDS[13];
	LEDS[15]=LEDS[12];

	LEDS[11]=0;
	LEDS[10]=0;
	LEDS[9]=0;
	LEDS[14]=0;
	LEDS[13]=0;
	LEDS[12]=0;

    switch(rand() % 16) {
        case 0:
			LEDS[11]=255;
		break;
        case 1:
			LEDS[10]=255;
		break;
        case 2:
			LEDS[9]=255;
		break;
		case 3:
			LEDS[14]=255;
		break;
		case 4:
			LEDS[13]=255;
		break;
		case 5:
			LEDS[12]=255;
		break;
	}
	if(count>400) {
		count=0;
		return queueNext(data->next);
	}
	return queueNext(data);
}


absolutetime_t stareV(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		if(i==4 || i==7 || i==16 || i==19) {
			LEDS[i] = data->ExtraData;
		} else {
			LEDS[i] = 0;
		}
	}
	return queueNext(data->next);
}

absolutetime_t stareH(void *d) {
	LedDanceFun *data = (LedDanceFun *)d;
	for(int i=0;i<TOTAL_LEDS;++i) {
		if(i==4 || i==5 || i==16 || i==17) {
			LEDS[i] = data->ExtraData;
			} else {
			LEDS[i] = 0;
		}
	}
	return queueNext(data->next);
}

LedDanceFun StareH  = {&stareH, 30000,255,0};
LedDanceFun StareV  = {&stareV, 30000,255,0};
LedDanceFun Dance1 = {&danceHorizonal,30000,1,0};
LedDanceFun Dance2 = {&danceVerticalCylon, 30000,1,0};

LedDanceFun Initial16 = {&turnAll		,30000,0,	0};
LedDanceFun Initial15 = {&danceVertical	,30000,0,	&Initial16};
LedDanceFun Initial14 = {&turnAll		,1000,0,	&Initial15};
LedDanceFun Initial13 = {&danceHorizonal	,30000,0,	&Initial14};
LedDanceFun Initial12 = {&turnOnRandom,300,0,	&Initial13};
LedDanceFun Initial11 = {&turnOnRandom,300,1,	&Initial12};
LedDanceFun Initial10 = {&danceVerticalCylon	,30000,0,	&Initial11};

LedDanceFun Initial9b = {&turnOnRandom,300,0,	&Initial10};
LedDanceFun Initial9 = {&danceRain		,2000,0,		&Initial9b};

LedDanceFun Initial8 = {&turnAll		,30000,0,	&Initial9};

LedDanceFun Initial7 = {&turnOnEveryN	,30000,8,	&Initial8};
LedDanceFun Initial6 = {&turnOnEveryN	,30000,4,	&Initial7};
LedDanceFun Initial5 = {&turnOnHalf		,30000,1,	&Initial6};
LedDanceFun Initial4 = {&turnOnHalf		,30000,1,	&Initial5};
LedDanceFun Initial3 = {&turnOnHalf		,30000,0,	&Initial4};

LedDanceFun Initial2 = {&danceVerticalCylon	,30000,0,	&Initial8};

LedDanceFun Initial1  = {&turnAll		,30000,0,	&Initial2};

LedDanceFun Initial0 = {&turnOnOffRandom,300,1,	&Initial1};
LedDanceFun Initial  = {&turnAll		,30000,1,	&Initial0};
LedDanceFun InitialSH = {&stareH          ,30000,255, &Initial};
LedDanceFun InitialSV = {&stareV          ,30000,255, &InitialSH};

LedDanceFun InitialOFF0 = {&turnOnRandom,1000,0,	&InitialSV};

LedDanceFun InitialON = {&turnOnRandom,1000,255,	&InitialOFF0};
LedDanceFun InitialOFF = {&turnOnRandom,1000,0,	&InitialON};

LedDanceFun *DanceSequences[] = {
	&InitialOFF
	,&InitialON
	,&InitialOFF0
	,&InitialSV
	,&InitialSH
	,&Initial
	,&Initial1
	,&Initial2
	,&Initial3
	,&Initial4
	,&Initial5
	,&Initial7
	,&Initial8
	,&Initial9
	,&Initial9b
	,&Initial10
	,&Initial11
	,&Initial12
	,&Initial13
	,&Initial14
	,&Initial15
	,&Initial16
	,&Dance1
	,&Dance2
	,&StareH
	,&StareV
};

#define NUMBER_OF_DANCES (sizeof(DanceSequences)/sizeof(DanceSequences[0]))

absolutetime_t queueNext(LedDanceFun *data) {
	LedDanceFun *ds = 0;
	if(data) {
		LedTimer.callback_ptr = data->light;
		LedTimer.payload = data;
		return data->TimeOut;
	} else {
		int r = rand()%NUMBER_OF_DANCES;
		ds = DanceSequences[r];
		LedTimer.callback_ptr = ds->light;
		LedTimer.payload = ds;
		return ds->TimeOut;
	}
	
}


#if 0
bool PWMOn = true;
uint32_t onCount = 0;
uint32_t total = 0;

void PWM_handler_cb(void) {
	for(uint32_t i=0;i<TOTAL_LEDS;++i) {
		//if((LedCache&(1<<i))) {
			(*LedFunctions[i])(PWMOn);
		//}
	}
	uint32_t count =0 ;
	if(PWMOn) {
		onCount+=TCA0.SINGLE.CNT;
		count = 650;
	} else {
		count = 65000;
		//offCount+=TCA0.SINGLE.CNT;
	}
	total+=TCA0.SINGLE.CNT;
	PWM_0_load_counter(0);
	PWM_0_load_top(count);
	PWMOn = !PWMOn;

}

void setupPWM() {
	// Enable pin output
	PWM_0_enable_output_ch0();

	// Set channel 0 duty cycle value register value to specified value
	PWM_0_load_duty_cycle_ch0(0xFFFF);
	PWM_0_load_top(650);

	// Set counter register value
	PWM_0_load_counter(0);

	PWM_0_register_callback(PWM_handler_cb);
}
#endif

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	setupLeds();
	setupSlave();
	
	//dutyPct =1;
	//setupPWM();
	//PWM_handler_cb();
	
	ENABLE_INTERRUPTS();
	LedTimer.callback_ptr = stareV;
	LedTimer.payload = &InitialOFF;
	TIMER_0_timeout_create(&LedTimer, 30000);
	
	int led = 0;
	/* Replace with your application code */
	while (1) {
		if(LEDS[led]>0) {
			(*LedFunctions[led])(true);
		} else {
			(*LedFunctions[led])(false);
		}
		if(++led>=TOTAL_LEDS) {
			led=0;
		}
		if(NONE!=I2C_0_register_address && NONE!=I2C_0_cmd) {
			switch(I2C_0_register_address) {
				case 1:
					LedTimer.callback_ptr = turnAll;
					LedTimer.payload = &Initial;
					TIMER_0_timeout_create(&LedTimer, 1000);
					break;
				case 2:
					LedTimer.callback_ptr = turnAll;
					LedTimer.payload = &Dance1;
					TIMER_0_timeout_create(&LedTimer, 1000);
					break;
				case 3:
					LedTimer.callback_ptr = turnAll;
					LedTimer.payload = &Dance2;
					TIMER_0_timeout_create(&LedTimer, 1000);
					break;
			}
			I2C_0_cmd = NONE;
			I2C_0_register_address = NONE;
		}
		TIMER_0_timeout_call_next_callback();
	}
}
