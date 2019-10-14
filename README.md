# Introduction
Wall-ink was designed from the ground up to be a scalable, [secure](https://github.com/caedm/wall-ink/wiki/Device-security), and completely open solution to display conference room schedules at a university.  The product could, however, be developed to display other images for advertising, directions, staff directories, a weather station, retail pricing, etc.  Currently, the [wall-ink-server](https://github.com/caedm/wall-ink-server) can display schedules from the open source [Booked scheduler](https://www.bookedscheduler.com), any web published iCal file (Outlook / Exchange), and the Google Calendar API.  It can also [rotate through a collection of static images](https://github.com/caedm/wall-ink-server/wiki/Plugin-architecture#static-images-plugin).  With [additional development](https://github.com/caedm/wall-ink-server/wiki/Coding-a-new-plugin), other scheduler integrations or functions are possible.  The hardware simply displays whatever image it is given when it checks in with a wall-ink-server.  Development for putting images on the e-ink screen is made simpler by [allowing the image creation processing](https://github.com/caedm/wall-ink-server/wiki/Wall-ink-image-formats-and-image-handling) to occur on the server side, and not on the device.  Server-side image creation was a design choice for deploying a large fleet of these devices across mulitple buildings.  This way, changes can be made to what the devices are displaying without having to update firmware on dozens or hundreds of devices.  To date, we are aware of several other groups who have successfully written their own server-side software for wall-ink devices in various languages.  Links to other related wall-ink projects are found on the [additional wall ink projects](https://github.com/caedm/wall-ink/wiki/Additional-wall-ink-projects) page.

<img src="https://i.imgur.com/etozOAa.png" width="300"><img src="https://i.imgur.com/uxgWvmZ.png" width="236"><img src="https://i.imgur.com/bR7Etyk.png" width="283">

The original design parameters of this project were to display a room schedule electronically with no wires, at the cheapest possible price point.  No Ethernet, no power wires, in a low-cost footprint.  The resulting project has produced a AA powered WiFi E-ink device for under $100 (US), when parts are purchased in bulk, where the [batteries last a year or two](https://github.com/caedm/wall-ink/wiki/Circuit#batteries).  

The wall-ink project along with the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) includes everything necessary to build the entire e-ink display system. 

# Summary
The wall-ink project has several components:

* [Circuit and electronics](https://github.com/caedm/wall-ink/wiki/circuit), including Eagle schematic, PCB layout and gerber files
* List of necessary [parts](https://github.com/caedm/wall-ink/wiki/partslist) to build the project
* Arduino IDE [Firmware](https://github.com/caedm/wall-ink/wiki/firmware) for the ESP8266 microcontroller
* CAD files to 3D print the [Case](https://github.com/caedm/wall-ink/wiki/firmware)
* See the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) project for server side code
<img src="https://i.imgur.com/W1smB71.png">

# FAQ

## What hardware does wall-ink run on and where do I get it?
The wall-ink project is a hardware, firmware, and software build.  To date, we are not aware of any off-the-shelf commercial products that will do everything that the [wall-ink device](https://github.com/caedm/wall-ink/wiki/7.5%22-Wall-ink-device) does.  There are plenty of ESP8266 devices out there, including ones that sleep, but the hardware as described here is specifically designed to run the wall-ink firmware, sleep, use very little power, fit in the supplied 3D printable case, and update a Waveshare e-ink screen.

## Can I hire you to make the hardware for me?
Due to conflict of interest policies at the University we work for, we are not available for hire to create hardware for you, or to customize software features for you.  That being said, we have uploaded all of the design files and code, and reasonable documentation to do it on your own.  Many people have created this project on their own with positive results.

## What is an ESP8266 and why did you choose that microprocessor?
The [ESP8266 by Espressif Systems](https://en.wikipedia.org/wiki/ESP8266) is similar to an Arduino, but has 10uA deep-sleep function and built-in WiFi with a full TCP/IP stack.  The ESP8266 is also low cost, and fully compatible with the Arduino IDE.  So for under $3, you get a microprocessor that is perfect for this project.

## Why e-ink?  Is it a touch display?
In order to run the device for over a year on a set of AA batteries, we needed to use the lowest power screen possible. Since e-ink screens can hold their image even with the power removed, this is by far the lowest power option out there.  To keep the cost down, the type of [E-ink screens](https://en.wikipedia.org/wiki/E-ink) we chose from Waveshare / Good Display does not have touch screen capability.

## What e-ink screens are supported?
The real limitation of screen resolution support is the RAM available on the esp8266.  Some server-side plugins like the static image plugin and the simple text plugin can handle any screen size dynamically.  Other plugins like the booked and google plugins that use the general scheduling functions are hard-coded for Waveshare 7.5" screens at 640x384 and Waveshare 4.2" screens at 400x300 pixels.  There is currently no support for color displays.  We believe that there would be insufficient memory to support the larger color screens using the current method that the firmware has been coded.  The color screens also take a significant amount of time to update, and the batteries would not last sufficiently long for our use-case, so we did not pursue this option.

<img src="https://i.imgur.com/BPVGRtd.png" width="320">

## Why does a wall-ink device require a server?
Using a central server for controlling what is displayed on wall-ink devices was a design decision to ease the management of large numbers of devices.  If you only need to manage one e-ink device, running a server component might be overkill, but with dozens or hundreds of devices in the field, it is essential.  Tweaking what is being displayed on 100 or more screens can be done by changing a parameter on a website rather than updating firmware on dozens of devices across the enterprise.  Any wall-ink device pointed to a wall-ink-server can be configured do display ANY image at any time the next time in checks in.  There are also central management features such as looking at error logs, Nagios monitoring, voltage monitoring, and other such high-level tools, all made possible through this architecture.

## Can Wall-ink devices talk to a webserver using https?
The short answer is no.  There are libraries available for an esp8266 to talk to a webserver securely, but [there is not enough RAM onboard](https://github.com/caedm/wall-ink/wiki/Wall-ink-Memory#ram) to run the neccessary encryption code.  If security is a concern, please refer to the wiki articles about [device security](https://github.com/caedm/wall-ink/wiki/Device-security) and [wall-ink server security considerations](https://github.com/caedm/wall-ink-server/wiki/Security-considerations#image-security).

## Will a Wall-ink device connect to enterprise WiFi?
The short answer is maybe.  The ESP8266 WiFi client firmware has been tested to work with WPA2-PSK but does not appear to work with WPA2 Enterprise (including Eduroam). Some enterprises offer an additional "guest" network SSID with limited internal connectivity to handle embedded IOT devices as a solution to this problem.  Before going to the trouble of building this whole project and purchasing a screen, you could purchase an ESP8266 devkit and try connecting to your company's WiFi. 

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
Buy all the [loose parts](https://github.com/caedm/wall-ink/wiki/partslist) and solder them to the [printed circuit board](https://github.com/caedm/wall-ink/wiki/Printed-Circuit-Board).  You will also need to [3d print a case](https://github.com/caedm/wall-ink/wiki/7.5%22-Wall-ink-case) for the project.  If you want to start with the real thing, there is no reason why you can't just go straight to it.

<img src="https://i.imgur.com/KjgHKaQ.png" width="400">

## Building and flashing the firmware on to the ESP8266
See the detailed instructions in the [building and flashing firmware wiki article](https://github.com/caedm/wall-ink/wiki/firmware#Building) for instructions on uploading the firmware.

## Install and configure the web server code
See the documentation for the [wall-ink-server](https://github.com/caedm/wall-ink-server) companion project.  If desired, you could write a much simpler server that was not designed for an enterprise as long as it returned files in the correct ["wink format"](https://github.com/caedm/wall-ink-server/wiki/Wall-ink-image-formats-and-image-handling#wink-file-format).  The wall-ink-server project is designed to handle hundreds of devices using various plugins to collect data and present images to the screens.  It includes Nagios monitoring scripts, battery monitors and other tools to handle a large fleet of wall-ink devices.
## Configure the device
Use [Admin mode](https://github.com/caedm/wall-ink/wiki/Admin-mode) to set up the wireless client, the image key, and the URL to get images from.
