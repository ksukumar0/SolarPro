/*
   Wireless Sensor Networks Laboratory 2019 -- Group 1

   Technische Universität München
   Lehrstuhl für Kommunikationsnetze
http://www.lkn.ei.tum.de

copyright (c) 2019 Chair of Communication Networks, TUM

project: SolarPro

contributors:
 * Karthik Sukumar
 * Johannes Machleid

 This c-file is specifically designed for the base node.
 */

// Contiki-specific includes:
#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "net/rime/rime.h"	// Establish connections.
#include "net/netstack.h"	// Wireless-stack definitions
#include "lib/random.h"
#include "dev/leds.h"		// Use LEDs.
#include "core/net/linkaddr.h"
#include "dev/button-sensor.h" // User Button
#include "dev/adc-zoul.h"      // ADC
#include "dev/zoul-sensors.h"  // Sensor functions
#include "dev/sys-ctrl.h"

// Standard C includes:
#include <stdio.h>
#include <stdint.h>

//project headers
#include "helpers.h"
#include "nodeID.h"
#include "anemometer.h"
#include "routing.h"

#define TX_POWER 7
#define UNICASTTRASMITINTERVAL 5*CLOCK_SECOND

// int power_options[] =    {255,237,213,197,182,176,161,145,136,114,98 ,88 ,66 ,0};
// int power_dBm[] =        {7  ,5  ,3  ,1  ,0  ,-1 ,-3 ,-5 ,-7 ,-9 ,-11,-13,-15,-24};

/*---------------------------------------------------------------------------*/
#define READ_SENSOR_PERIOD          CLOCK_SECOND
#define ANEMOMETER_THRESHOLD_TICK   13  /**< (value*1.2) = 16 Km/h */
/*---------------------------------------------------------------------------*/

static struct etimer et;

/*---------------------------------------------------------------------------*/
/*  MAIN PROCESS DEFINITION  												 */
/*---------------------------------------------------------------------------*/

PROCESS(windSpeedThread, "Wind Speed Sensor Thread");
PROCESS(networkdiscoveryThread, "Network Discovery Thread");
PROCESS(uniCastThread, "Unicast Thread");
AUTOSTART_PROCESSES(&windSpeedThread, &networkdiscoveryThread, &uniCastThread);


/*---------------------------------------------------------------------------*/
    static void
wind_speed_callback(uint16_t value)
{
    /* This checks for instant wind speed values (over a second), the minimum
     * value is 1.2 Km/h (one tick), as the reference is 2.4KM/h per rotation, and
     * the anemometer makes 2 ticks per rotation.  Instant speed is calculated as
     * multiples of this, so if you want to check for 16Km/h, then it would be 13
     * ticks
     */
    printf("*** Wind speed over threshold (%u ticks)\n", value);
}

/*** Wind Speed Sensor THREAD ***/
PROCESS_THREAD (windSpeedThread, ev, data)
{

    PROCESS_BEGIN ();

    static uint16_t wind_speed;
    static uint16_t wind_speed_avg;
    static uint16_t wind_speed_avg_2m;
    static uint16_t wind_speed_max;

    printf("Anemometer test, integration period %u\n",
            ANEMOMETER_AVG_PERIOD);

    /* Register the callback handler when thresholds are met */
    REGISTER_ANEMOMETER_INT(wind_speed_callback);

    /* Enable the sensors, this has to be called before any of the interrupt calls
     * like the ones below
     */
    SENSORS_ACTIVATE(anemometer);

    /* And the upper threshold value to compare and generate an interrupt */
    anemometer.configure(ANEMOMETER_INT_OVER, ANEMOMETER_THRESHOLD_TICK);


    etimer_set(&et, READ_SENSOR_PERIOD);

    while (1)
    {

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        /* If timer expired, print sensor readings */

        wind_speed = anemometer.value(ANEMOMETER);
        wind_speed_avg = anemometer.value(ANEMOMETER_AVG);
        wind_speed_avg_2m = anemometer.value(ANEMOMETER_AVG_X);
        wind_speed_max = anemometer.value(ANEMOMETER_MAX);

        /*
           printf("Wind speed: %u.%u km/h ", wind_speed/1000, (wind_speed % 1000)/100);
           printf("(%u.%u km/h avg, %u.%u km/h 2m avg, %u.%u km/h max)\n\n", wind_speed_avg/1000, (wind_speed_avg % 1000)/100,
           wind_speed_avg_2m/1000, (wind_speed_avg_2m % 1000)/100, wind_speed_max/1000, (wind_speed_max % 1000)/100);
           */
        etimer_reset(&et);

    }

    PROCESS_END ();
}

PROCESS_THREAD (networkdiscoveryThread, ev, data)
{
    PROCESS_BEGIN();

    // Configure your team's channel (11 - 26).
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,11);
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, TX_POWER); //Set up Tx Power
    openBroadcast();
    setUpRtable();
    printf("My RIME ID is %x Node ID is %d\n", getMyRIMEID()->u16, getMyNodeID());

    /* Configure the user button */
    button_sensor.configure(BUTTON_SENSOR_CONFIG_TYPE_INTERVAL, CLOCK_SECOND);
    static struct etimer et_broadCastOver;
    etimer_set(&et_broadCastOver, 10*CLOCK_SECOND);
    static int i = 0;

    while(1) {
        PROCESS_WAIT_EVENT();
        // check if button was pressed
        if(ev == sensors_event)
        {
            if(data == &button_sensor)
            {
                if( button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
                        BUTTON_SENSOR_PRESSED_LEVEL )
                {
                    //initNetworkDisc();
                }
            }
        }//end if(ev == sensors_event)
        else if (ev == PROCESS_EVENT_TIMER)
        {
            process_post(&uniCastThread, PROCESS_EVENT_MSG, i++);
            etimer_reset(&et_broadCastOver);
        }
    }//end while(1)

    PROCESS_END();
}

PROCESS_THREAD (uniCastThread, ev, data)
{
    PROCESS_BEGIN();

    // Configure your team's channel (11 - 26).
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,11);
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, TX_POWER); //Set up Tx Power

    static struct etimer et_unicastTransmit;
    etimer_set(&et_unicastTransmit, UNICASTTRASMITINTERVAL);

    /* Configure the user button */
    button_sensor.configure(BUTTON_SENSOR_CONFIG_TYPE_INTERVAL, CLOCK_SECOND);

    while(1) {
        PROCESS_WAIT_EVENT();
        // check if button was pressed
        //if(ev == START_UNICAST)
        //{
        //    etimer_set(&et_unicastTransmit, UNICASTTRASMITINTERVAL);
        //}//end if(ev == sensors_event)
        if (ev == PROCESS_EVENT_TIMER)
        {
            etimer_reset(&et_unicastTransmit);
            leds_toggle(LEDS_BLUE);
            print_active_procs();
        }
        else if (ev == PROCESS_EVENT_MSG)
        {
            printf("Received  MSG!!! is %d\n",data);
        }
    }//end while(1)

    PROCESS_END();
}

