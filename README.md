# Introduction
Wall-ink was architected from the ground up to be a scalable, [secure](https://github.com/caedm/wall-ink/wiki/Device-security), and completely open solution to display room schedules at a university.  The product could, however, be developed to display other images for advertising, directions, staff directories, retail pricing, etc.  Currently, the [wall-ink-server](https://github.com/caedm/wall-ink-server) can display schedules from the open source [Booked scheduler](https://www.bookedscheduler.com), any web published iCal file (Outlook / Exchange), and Google Calendar API.  It can also [rotate through a collection of static images](https://github.com/caedm/wall-ink-server/wiki/Plugin-architecture#static-images-plugin).  With [additional development](https://github.com/caedm/wall-ink-server/wiki/Coding-a-new-plugin), other scheduler integrations or functions are possible.  The hardware simply displays whatever image it is given when it checks in with a wall-ink-server.  Development for putting images on the e-ink screen is made simpler by [allowing the image creation processing](https://github.com/caedm/wall-ink-server/wiki/Wall-ink-image-formats-and-image-handling) to occur on the server side, and not on the device.  To date, we are aware of two other groups who have written their own server-side software for the wall-ink devices in both Python and PHP, but we are waiting for those projects to be posted on GitHub.

<img src="https://i.imgur.com/etozOAa.png" width="300"><img src="https://i.imgur.com/uxgWvmZ.png" width="236"><img src="https://i.imgur.com/bR7Etyk.png" width="283">

The original design parameters of this project were to display a room schedule electronically with no wires, at the cheapest possible price point.  No Ethernet, no power wires, in a low-cost footprint.  The resulting project has produced a AA powered WiFi E-ink device for under $100 (US), when parts are purchased in bulk, where the [batteries last a year or two](https://github.com/caedm/wall-ink/wiki/Circuit#batteries).  

The wall-ink project along with the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) include everything necessary to build the entire e-ink display system. 

# Summary
The wall-ink project has several components:

* [Circuit and electronics](https://github.com/caedm/wall-ink/wiki/circuit), including Eagle schematic, PCB layout and gerber files
* List of necessary [parts](https://github.com/caedm/wall-ink/wiki/partslist) to build the project
* Arduino IDE [Firmware](https://github.com/caedm/wall-ink/wiki/firmware) for the ESP8266 microcontroller
* CAD files to 3D print the [Case](https://github.com/caedm/wall-ink/wiki/firmware)
* See the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) project for server side code
<img src="https://i.imgur.com/W1smB71.png">

# FAQ
## What is an ESP8266 and why did you choose that microprocessor?
The [ESP8266 by Espressif Systems](https://en.wikipedia.org/wiki/ESP8266) is similar to an Arduino, but has 10uA deep-sleep function and built-in WiFi with a full TCP/IP stack.  The ESP8266 is also low cost, and fully compatible with the Arduino IDE.  So for under $3, you get a microprocessor that is perfect for this project.

## Why e-ink?  Is it a touch display?
In order to run the device for over a year on a set of AA batteries, we needed to use the lowest power screen possible. Since e-ink screens can hold their image even with the power removed, this is by far the lowest power option out there.  To keep the cost down, the type of [E-ink screens](https://en.wikipedia.org/wiki/E-ink) we chose from Waveshare / Good Display does not have touch screen capability.

## What e-ink screens are supported?
The real limitation of screen resolution support is the RAM available on the esp8266.  Some server-side plugins like the static image plugin and the simple text plugin can handle any screen size dynamically.  Other plugins like the booked and google plugins that use the genral scheduling functions are hard-coded for Waveshare 7.5" screens at 640x384 and Waveshare 4.2" screens at 400x300 pixels.  There is currently no support for color displays.

<img src="https://i.imgur.com/BPVGRtd.png" width="320">

## What skills and tools  will I need to complete this project?
Some basic electronics experience could be useful if building a [development board prototype](https://github.com/caedm/wall-ink/wiki/development-board-prototype). Building an actual [wall-ink device](https://github.com/caedm/wall-ink/wiki/wall-ink-device) will require some basic soldering and 3d printing, and some electronics experience would be useful to help identify and purchase all the right [parts](https://github.com/caedm/wall-ink/wiki/partslist). The software end of the project will require the use of the Arduino IDE to flash the firmware, and a simple web server to run the [wall-ink-server](https://github.com/caedm/wall-ink-server). The wall-ink-server project has only been tested with Apache, but should work with other server software that supports PHP.

# Getting started
To get a wall-ink device up and running, you will need to build a hardware device and flash it with the latest firmware.  Then you will need to install a [small web service] to host the content for your device to display.  Last, you will need to [configure your new hardware device](https://github.com/caedm/wall-ink/wiki/Admin-mode) to access your wireless network and find your webserver.
## Building the hardware
There are a couple different methods to get started with wall-ink hardware
### Option 1: 
Purchase an esp8266 development board, and a Waveshare e-ink display with HAT and make a [development board prototype](https://github.com/caedm/wall-ink/wiki/Development-board-prototype).  This method will get you up and running quickly with a minimal cost for a proof-of-concept project.  You will be able to power and flash the development board via USB from your computer.  These development boards are designed to be easy to use, but are not meant for battery operated production devices.  The integrated USB to serial chip on the development board alone will drain your batteries in a matter of days.  If you want your device to operate for years on end off of batteries, you will need to build the wall-ink circuit that has been specifically designed for low power draw.
<img src="https://i.imgur.com/3qNuTQ9.png" width="400">
<img src="https://i.imgur.com/OLuOBQk.png" width="400">


### Option 2:  
Buy all the [loose parts](https://github.com/caedm/wall-ink/wiki/partslist) and [[build the wall-ink circuit]].  If you want to start with the real thing, there is no reason why you can't just go straight to it.

<img src="https://i.imgur.com/KjgHKaQ.png" width="400">

## Building and flashing the firmware on to the ESP8266
See the detailed instructions in the [building and flashing firmware wiki article](https://github.com/caedm/wall-ink/wiki/firmware#Building) for instructions on uploading the firmware.

## Install and configure the web server code
See the documentation for the [wall-ink-server](https://github.com/caedm/wall-ink-server) companion project.  If desired, you could write a much simpler server that was not designed for an enterprise as long as it returned files in the correct ["wink format"](https://github.com/caedm/wall-ink-server/wiki/Wall-ink-image-formats-and-image-handling#wink-file-format).  The wall-ink-server project is designed to handle hundreds of devices using various plugins to collect data and present images to the screens.  It includes Nagios monitoring scripts, battery monitors and other tools to handle a large fleet of wall-ink devices.
## Configure the device
Use [Admin mode](https://github.com/caedm/wall-ink/wiki/Admin-mode) to set up the wireless client, the image key, and the URL to get images from.
