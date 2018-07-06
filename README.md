# Introduction
Wall-ink was architected from the ground up to be an enterprise friendly, secure, and completely open solution to display room schedules at a University.  The product could however be developed to display other images for advertising, directions, staff directories, retail pricing, etc.  Currently, the [wall-ink-server](https://github.com/caedm/wall-ink-server) can display schedules from the open source [Booked scheduler](https://www.bookedscheduler.com), and Google Calendar.  It can also display static images.  With additional development, other scheduler integrations or functions are possible.  The hardware simply displays whatever image it is given.  

<img src="https://i.imgur.com/etozOAa.png" width="300"><img src="https://i.imgur.com/uxgWvmZ.png" width="236"><img src="https://i.imgur.com/bR7Etyk.png" width="283">

The original design parameters were to display a room schedule electronically with no wires, at cheapest possible price point.  No Ethernet, no power wires, in a low-cost footprint.  The resulting project has produced a AA powered WiFi E-ink device for under $100 (US), when parts are purchased in bulk, where the batteries last a year or two.  

The wall-ink project along with the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) include everything necessary to build the entire e-ink display system. 
 
# Summary
The wall-ink project has several components:

* [Circuit and electronics](https://github.com/caedm/wall-ink/wiki/circuit), including Eagle schematic, PCB layout and gerber files
* List of necessary [parts](https://github.com/caedm/wall-ink/wiki/partslist) to build the project
* Arduino IDE [[Firmware]] for the ESP8266 microcontroller
* [[CAD files]] to 3D print case
* See the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) project project for server side code

# Getting started
To get a wall-ink device up and running, you will need to build a hardware device and flash it with the latest firmware.  Then you will need to install a [small web service] to host the content for your device to display.  Last, you will need to [configure your new hardware device] to access your wireless network and find your webserver.
## Building the hardware
There are a couple different methods to get started with wall-ink hardware
### Option 1: 
Purchase an esp8266 development board, and a Waveshare e-ink display with HAT and make a [prototype on a breadboard].  This method will get you up and running quickly with a minimal cost for a proof-of-concept project.  You will be able to power and flash the development board via USB from your computer.  These development boards are designed to be easy to use, but are not meant for battery operated production devices.  The integrated USB to serial chip on the development board alone will drain your batteries in a matter of days.  If you want your device to operate for years on end off of batteries, you will need to build the wall-ink circuit that has been specifically designed for low power draw.
<img src="https://i.imgur.com/EimzrMy.png" width="400">
### Option 2:  
Buy all the [loose parts](https://github.com/caedm/wall-ink/wiki/partslist) and build the wall-ink circuit.  If you want to start with the real thing, there is no reason why you can't just go straight to it.
## Flashing the firmware on to the ESP8266

## Install and configure the web server code

## Configure the device
Use [Admin mode] to set up the wireless client, the image key, and the URL to get images from.
