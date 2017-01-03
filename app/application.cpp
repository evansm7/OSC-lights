/* Copyright (c) 2015  Matt Evans
 * Simple demo program for osc.cpp OSC parser, using Sming.
 * Created 17th June 2015.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "osc.h"


#define WIFI_SSID "YOUR_SSID_HERE"
#define WIFI_PWD "YOUR PASSWORD HERE"


/*****************************************************************************/
/* UDP server for OSC */
void onReceive(UdpConnection& connection, char *data, int size,
	       IPAddress remoteIP, uint16_t remotePort);

const uint16_t OSCPort = 10000;
UdpConnection udp(onReceive);

#define LED_PIN 	2
#define BUTTON_PIN 	0

/* I was using the DriverPWM class, instantiated as:
 * DriverPWM ledPWM(5000);	// 200Hz
 * (Additionally, LED_BRIGHT_FULL is top of 0-255 range.)
 *
 * However, switched to trying HardwarePWM as that was purported to be less
 * jittery.  It is much better, though not perfect.
 */
static uint8_t 	pins[8] = { LED_PIN };
static HardwarePWM 	ledPWM(pins, 1);
static Timer 		buttonTimer;
static int 		button_state = 0;

#define LED_BRIGHT_FULL 22222
static uint16_t 	led_brightness = 0;

/*****************************************************************************/

/* Set the LED into an on/off/dim/whatever state, as called from the
 * button-change code or the OSC callback.
 */
static void update_led(uint16_t b)
{
	Serial.printf("updateled to %d\r\n", b);
	led_brightness = b;
	// Crappy workaround:
	if (b != 0) {
		ledPWM.analogWrite(LED_PIN, b);
	} else {
		ledPWM.analogWrite(LED_PIN, 2);
		ledPWM.analogWrite(LED_PIN, 1);
		ledPWM.analogWrite(LED_PIN, 0);
	}
}

/* Callback function from the OSC */
static void cb_cf(const char *prefix, int argc, osc_args_t argv)
{
	// I'm expecting one float argument, 0-127.
	if (argv[0].type == FLOAT) {
		Serial.printf("OSC_CB:  Arg is, as int, %d\r\n", (int)argv[0].u.f);

		update_led((int)(argv[0].u.f * (float)LED_BRIGHT_FULL / 127.0));
	} else {
		debugf("OSC_CB bad argument type %d", argv[0].type);
	}
}

void onReceive(UdpConnection& connection, char *data, int size,
	       IPAddress remoteIP, uint16_t remotePort)
{
	osc_parse_msg(data);
}

void onConnected()
{
	udp.listen(OSCPort);

	Serial.println("\r\n=== Listening on: ");
	Serial.print(WifiStation.getIP().toString()); Serial.print(":"); Serial.println(OSCPort);
	Serial.println(" ===\r\n");
}

void connectFail()
{
	debugf("--- Connect failure ---\r\n");
	WifiStation.waitConnection(onConnected, 20, connectFail);
}

static void button_pushed()
{
	debugf("Button press");
	// Toggle LED on/off.
	if (led_brightness > (LED_BRIGHT_FULL/2))
		update_led(0);
	else
		update_led(LED_BRIGHT_FULL);
}

/* Periodically called from button timer. */
static void button_tcb()
{
	int i = !digitalRead(BUTTON_PIN);
	static int samples = 20;
	int new_state = button_state;

	// Do some really basic button debouncing:
	if (i) {
		if (--samples == 0) {
			// Did N samples at the same state
			new_state = 1;
			samples = 20;
		}
	} else {
		// No debouncing for unpressed
		new_state = 0;
		samples = 20;
	}
	if (new_state != button_state) {
		button_state = new_state;
		if (new_state)
			button_pushed();
	}
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true);

	wifi_set_sleep_type(LIGHT_SLEEP_T);

	/* We respond to all of the following OSC addresses in the same way:
	 * change the LED brightness.
	 */
	osc_reg_handler("/crossfader", cb_cf);
	osc_reg_handler("/leftButton", cb_cf);
	osc_reg_handler("/rightButton", cb_cf);
	osc_reg_handler("/centerButton", cb_cf);

	ledPWM.analogWrite(LED_PIN, 0);

	pinMode(BUTTON_PIN, INPUT);
	buttonTimer.initializeMs(10, button_tcb).start();

	Serial.println("\r\nInitialisation done.\r\n");

	WifiAccessPoint.enable(false);
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.waitConnection(onConnected, 20, connectFail);

	wifi_set_sleep_type(LIGHT_SLEEP_T);
	Serial.println("Wifi enabled.\r\n");
}
