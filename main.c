#include <xil_printf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

const volatile uint32_t TCR_OFFSET = 2;
const volatile uint32_t TCSR_OFFEST = 0;
//#define BUTTONS (* (unsigned volatile *) 0x40000000)
#define JB (* (unsigned volatile *) 0x40002000)
#define JB_DDR (* (unsigned volatile *) 0x40002004)
#define AN (* (unsigned volatile *) 0x40020008)
#define LEDS (*(unsigned volatile *)0x40000000)
#define DPSEG (* (unsigned volatile *) 0x40020000)
#define SWITCHES (* (unsigned volatile *) 0x40007000)
//Hardware Timer Channel 0 - Ultrasonic Sensor
#define TCSR0     (* (unsigned volatile *) 0x40009000) //Timer Control & Status Register Channel 0
#define TCR0      (* (unsigned volatile *) 0x40009008) //Timer Counter Register 0
//Hardware Timer Channel 1 - SSEG
#define TCSR1     (* (unsigned volatile *) 0x40009010) //Timer Control & Status Register Channel 1
#define TCR1      (* (unsigned volatile *) 0x40009018) //Timer Counter Register 1
#define JXADC_CH6 (*(volatile unsigned *)0x44a10258)
#define JXADC_CH14 (*(volatile unsigned *)0x44a10278)
#define JXADC_CH7 (*(volatile unsigned *)0x44a1025C)

// Function declarations - implemented below
void timer_us(unsigned t);
void seg_disp(uint8_t data[4]);
//void update_leds_from_adc(unsigned adc_value, unsigned base_adc_value);
void update_leds_from_adc(unsigned adc_value_left, unsigned base_adc_value_left, unsigned adc_value_right, unsigned base_adc_value_right, unsigned adc_value_center, unsigned base_adc_value_center);

int main() {
/////////////////////////////////////////////
    // Setting Pin Directions and turning off SSEG
    AN = ~1;
    LEDS = 0x0000; // Turn off all LEDs

    // Initializing Timer Values
    TCSR0 = 0b010010010001;
    TCR0 = 0x00000000;

    // Tracking Variables
    uint8_t total_count = 0;
    uint8_t left_count = 0;
    uint8_t center_count = 0;
    uint8_t right_count = 0;

    uint8_t anodeArray[4];
    uint8_t positionCounts[4];

    uint8_t left_coil_value = 120; // mV
    uint8_t center_coil_value = 190; // mV
    uint8_t right_coil_value = 60; // mV
    _Bool left_detected = false;
    _Bool center_detected = false;
    _Bool right_detected = false;

    uint8_t coil_value;
	uint8_t coil_value_adc;

    uint8_t sevenSegLUT[16] = {
        0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
        0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E
    };

    while(1) {

    	seg_disp(anodeArray);

        uint8_t curr_left_coil_value = ((JXADC_CH6 >> 4) * 244) / 1000;
        uint8_t curr_center_coil_value = ((JXADC_CH14 >> 4) * 244) / 1000;
        uint8_t curr_right_coil_value = ((JXADC_CH7 >> 4) * 244) / 1000;

        if (curr_left_coil_value < left_coil_value)
        {
        	coil_value = left_coil_value;
        	coil_value_adc = curr_left_coil_value;

        	if(!left_detected)
        	{
        		left_detected = true;
        		total_count++;
        		left_count++;
                timer_us(1000000);
        	}
        }
        else
        {
        	left_detected = false;
        }

        if (curr_center_coil_value < center_coil_value)
        {
        	coil_value = center_coil_value;
        	coil_value_adc = curr_center_coil_value;

        	if(!center_detected)
        	{
        		center_detected = true;
				total_count++;
				center_count++;
				timer_us(1000000);
        	}
        }
        else
        {
        	center_detected = false;
        }

        if (curr_right_coil_value > right_coil_value)
        {
        	coil_value = right_coil_value;
        	coil_value_adc = curr_right_coil_value;

        	if(!right_detected)
        	{
				right_detected = true;
				total_count++;
				right_count++;
				timer_us(1000000);
        	}
        }
        else
        {
        	right_detected = false;
        }

        positionCounts[0] = total_count % 16;
        positionCounts[1] = right_count % 16;
        positionCounts[2] = center_count % 16;
        positionCounts[3] = left_count % 16;

        for (int i = 0; i < 4; i++) {
            anodeArray[i] = sevenSegLUT[positionCounts[i]];
        }
        if (left_detected) // left detected
        {
            anodeArray[3] &= 0x7F; // Turns on DP 3
        }

        if (center_detected) // center detected
        {
            anodeArray[2] &= 0x7F; // Turns on DP 2
        }

        if (right_detected) // right detected
        {
            anodeArray[1] &= 0x7F; // Turns on DP 1
        }


        update_leds_from_adc(curr_left_coil_value, left_coil_value, curr_right_coil_value, right_coil_value, curr_center_coil_value, center_coil_value);

        //update_leds_from_adc(coil_value_adc, coil_value);

        xil_printf("\nadc_value_left: %u\n", curr_left_coil_value);
        xil_printf("\nadc_value_center: %u\n", curr_center_coil_value);
        xil_printf("\nadc_value_right: %u\n", curr_right_coil_value);
        //xil_printf("\ndisplay: %u %u %u\n", left_count, center_count, right_count);
        timer_us(500000);
    }

}

void timer_us(unsigned t){
    volatile unsigned cntr1;
    while(t--)
        for( cntr1=0; cntr1 < 4; cntr1++);
}

// This function displays Data to the SSEG
void seg_disp(uint8_t data[4]){
    static uint8_t digit = 1;
    static uint16_t cnt = 0; // flash counter

    if(digit == 1){
        // Left most anode on and display data[3]
        if(cnt <= 300){
            AN = 0xF;
        }
        else{
            AN = 0x7;
        }
        DPSEG = data[3];
    }
    else if(digit == 2){
        // 2nd left most anode on and display data[2]
        if(cnt <= 300){
            AN = 0xF;
        }
        else{
            AN = 0xB;

        }
        DPSEG = data[2];
    }
    else if(digit == 3)
    {
        // 2nd right most anode on and display data[1]

        if(cnt <= 300){
            AN = 0xF;
        }

        else{
            AN = 0xD;
        }
        DPSEG = data[1];
    }
    else if(digit == 4)
    {
        // Right most anode on and display data[0]
        if(cnt <= 300){
            AN = 0xF;
        }
        else{
            AN = 0xE;
        }

        DPSEG = data[0];
        digit = 0;
    }
    // Increment count
    digit ++;
    cnt ++;
    if(cnt == 1000)
    {
    cnt = 0;
    }
}
/*
void update_leds_from_adc(unsigned adc_value, unsigned base_adc_value)
{
    int adc_drop = base_adc_value - adc_value;
    if (adc_drop < 0) adc_drop = 0;

    uint8_t leds_on = adc_drop / 2;
    if (leds_on > 16) leds_on = 16; // Clamp to 16 max

    switch (leds_on)
    {
        case 0:  LEDS = 0x0000; break;
        case 1:  LEDS = 0x0001; break;
        case 2:  LEDS = 0x0003; break;
        case 3:  LEDS = 0x0007; break;
        case 4:  LEDS = 0x000F; break;
        case 5:  LEDS = 0x001F; break;
        case 6:  LEDS = 0x003F; break;
        case 7:  LEDS = 0x007F; break;
        case 8:  LEDS = 0x00FF; break;
        case 9:  LEDS = 0x01FF; break;
        case 10: LEDS = 0x03FF; break;
        case 11: LEDS = 0x07FF; break;
        case 12: LEDS = 0x0FFF; break;
        case 13: LEDS = 0x1FFF; break;
        case 14: LEDS = 0x3FFF; break;
        case 15: LEDS = 0x7FFF; break;
        case 16: LEDS = 0xFFFF; break;
        default:
        	LEDS = LEDS;
        	break;
    }
}

*/
void update_leds_from_adc(unsigned adc_value_left, unsigned base_adc_value_left,
                          unsigned adc_value_right, unsigned base_adc_value_right,
                          unsigned adc_value_center, unsigned base_adc_value_center)
{
    int adc_drop_left = base_adc_value_left - adc_value_left;
    int adc_drop_center = base_adc_value_center - adc_value_center;
    int adc_drop_right = base_adc_value_right - adc_value_right;

    if (adc_drop_left < 0)
    {
        adc_drop_left = 0;
    }
    if (adc_drop_center < 0)
    {
        adc_drop_center = 0;
    }
    if (adc_drop_right < 0)
    {
        adc_drop_right = 0;
    }

    uint8_t leds_on_l = adc_drop_left / 6;
    uint8_t leds_on_r = adc_drop_right / 2;
    uint8_t leds_on_c = adc_drop_center / 4;

    if (leds_on_c > 16) leds_on_c = 16; // Clamp to 16 max
    if (leds_on_r > 16) leds_on_r = 16; // Clamp to 16 max
    if (leds_on_l > 16) leds_on_l = 16; // Clamp to 16 max

    uint16_t leds_left = 0x0000;
    uint16_t leds_right = 0x0000;
    uint16_t leds_center = 0x0000;

    // Set LEDs for left
    switch (leds_on_l)
    {
        case 0: leds_left = 0x0000; break;
        case 1: leds_left = 0x0001; break;
        case 2: leds_left = 0x0003; break;
        case 3: leds_left = 0x0007; break;
        case 4: leds_left = 0x000F; break;
        case 5: leds_left = 0x001F; break;
        case 6: leds_left = 0x003F; break;
        case 7: leds_left = 0x007F; break;
        case 8: leds_left = 0x00FF; break;
        case 9: leds_left = 0x01FF; break;
        case 10: leds_left = 0x03FF; break;
        case 11: leds_left = 0x07FF; break;
        case 12: leds_left = 0x0FFF; break;
        case 13: leds_left = 0x1FFF; break;
        case 14: leds_left = 0x3FFF; break;
        case 15: leds_left = 0x7FFF; break;
        case 16: leds_left = 0xFFFF; break;
        default: leds_left = 0x0000; break;
    }

    // Set LEDs for right
    switch (leds_on_r)
    {
        case 0: leds_right = 0x0000; break;
        case 1: leds_right = 0x0001; break;
        case 2: leds_right = 0x0003; break;
        case 3: leds_right = 0x0007; break;
        case 4: leds_right = 0x000F; break;
        case 5: leds_right = 0x001F; break;
        case 6: leds_right = 0x003F; break;
        case 7: leds_right = 0x007F; break;
        case 8: leds_right = 0x00FF; break;
        case 9: leds_right = 0x01FF; break;
        case 10: leds_right = 0x03FF; break;
        case 11: leds_right = 0x07FF; break;
        case 12: leds_right = 0x0FFF; break;
        case 13: leds_right = 0x1FFF; break;
        case 14: leds_right = 0x3FFF; break;
        case 15: leds_right = 0x7FFF; break;
        case 16: leds_right = 0xFFFF; break;
        default: leds_right = 0x0000; break;
    }

    // Set LEDs for center
    switch (leds_on_c)
    {
        case 0: leds_center = 0x0000; break;
        case 1: leds_center = 0x0001; break;
        case 2: leds_center = 0x0003; break;
        case 3: leds_center = 0x0007; break;
        case 4: leds_center = 0x000F; break;
        case 5: leds_center = 0x001F; break;
        case 6: leds_center = 0x003F; break;
        case 7: leds_center = 0x007F; break;
        case 8: leds_center = 0x00FF; break;
        case 9: leds_center = 0x01FF; break;
        case 10: leds_center = 0x03FF; break;
        case 11: leds_center = 0x07FF; break;
        case 12: leds_center = 0x0FFF; break;
        case 13: leds_center = 0x1FFF; break;
        case 14: leds_center = 0x3FFF; break;
        case 15: leds_center = 0x7FFF; break;
        case 16: leds_center = 0xFFFF; break;
        default: leds_center = 0x0000; break;
    }

    // Combine the LED values for left, right, and center using bitwise OR
    LEDS = leds_left | leds_right | leds_center;
}



