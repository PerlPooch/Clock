# Hardware

You can power Clock via the NodeMCU micro-USB port, but it is preferable to provide +5V power via the on-board connector or terminal J1. Driving the LEDs brighter than 50% when powered via the micro-USB connector may damage the NodeMCU.

A ring of 60 WS2812B LEDs are connected to J5 or J2 (but not both). Pin 1 is ground, Pin 2 is data, Pin 3 is +5. See [[RGB Clock Controller/Schematic.pdf][The Schematic]] for details. 

Dual connectors J3 are provided for an optional small OLED display. Select the header that matches the orientation of the display.

# Software 2.11

## Wifi Configuration

If there is no configuration present or Clock is powered-on with button (SW3) depressed, Clock enters WiFi configuration mode.

In this mode, Clock will create a WiFi access point with the SSID `Clock xx:xx:xx:xx:xx:xx`. Join this AP with the password `12345678` and you will be presented with menus through which you can select and join an Internet-connected AP.

Once configured, Clock will reboot.

## USB-Serial Access

Clock will provide a serial interface via USB at 115200/8/n/1.

## OpenWeather

If you have an OpenWeather key, Clock will obtain the weather by making requests to 

    GET http://api.openweathermap.org/data/2.5/weather

using the configured key. You must set `owAppKey`, `latitude`, and `longitude`.

## Start Up

Clock will test the display, connect to the configured WiFi AP, and retrieve the current date and time via NTP. 

## Configuration

You can reach Clock via HTTP at its address. If you don't know the address that was assigned, you can enter WiFi Configuration mode and view the assigned IP address. Alternatively, the assigned IP address is available via the Serial connection upon start up.

### Query Values and Status

Making a GET request to / will return a JSON payload of the current configuration. 

    http://<clockip>/

| Field                | Meaning / Range                                                              |
| :------------------- | :--------------------------------------------------------------------------- |
| **"sleepRemaining"** | `<int>` – Remaining **sleep time** in seconds                                |
| **"timezone"**       | `<int>` – **Timezone offset** from OpenWeather data                          |
| **"nextNTPUpdate"**  | `<int>` – Seconds until the next **NTP time update**                         |
| **"nextOWUpdate"**   | `<int>` – Seconds until the next **OpenWeather update**                      |
| **"temperature"**    | `<int>` – Current **temperature (°F)**                                       |
| **"lastOWResponse"** | `<int>` – Last **HTTP response code** from OpenWeather                       |
| **"time"**           | `<string>` – Current **time**                                                |
| **"display"**        | `<string>` – Display state (**Day**, **Night**, **Sleep**, or **Preparing**) |
| **"version"**        | `<string>` – Clock **firmware version**                                      |

### Set Values

Making a GET request with a query-string can configure the options. 

For example, to set the dim intensity and bright intensity for both daytime and nighttime modes:

    http://clockip/?dim=7&bright=20&dimNight=7&brightNight=20

| Option             | Meaning / Range                                                  |
| :----------------- | :--------------------------------------------------------------- |
| **"dim"**          | `0 .. 100` – Brightness of standard dots during **Day time**     |
| **"bright"**       | `0 .. 100` – Brightness of tick dots during **Day time**         |
| **"dimNight"**     | `0 .. 100` – Brightness of standard dots during **Night time**   |
| **"brightNight"**  | `0 .. 100` – Brightness of tick dots during **Night time**       |
| **"temp"**         | `0 .. 100` – Brightness of temperature dot when **sleeping**     |
| **"hue"**          | `0 .. 100` – Universal **color shift** to apply                  |
| **"coldHue"**      | `0 .. 100` – Color of temperature dot when **temperature < 60°** |
| **"warmHue"**      | `0 .. 100` – Color of temperature dot when **temperature > 60°** |
| **"nightHour"**    | `0 .. 23` – Hour to **start Night time**                         |
| **"dayHour"**      | `0 .. 23` – Hour to **end Night time**                           |
| **"owAppKey"**     | `<string>` – **OpenWeather API Key**                             |
| **"latitude"**     | `<float>` – Location **Latitude**                                |
| **"longitude"**    | `<float>` – Location **Longitude**                               |
| **"displaySleep"** | `<int>` – Sleep the display for this many **seconds**            |
| **"reboot"**       | `<>` – **Reboot**                              |


## OTA Firmware Update

    GET http://<clockip>/_ac/update

## WiFi Configuration Changes

    GET http://<clockip>/_ac/


