# Introduction
Wall-ink was architected from the ground up to be an enterprise friendly, secure, and completely open solution to display room schedules at a University.  The product could however be used to display other images for advertising, directions, staff directories, retail pricing, etc.  

<img src="https://i.imgur.com/etozOAa.png" width="300"><img src="https://i.imgur.com/uxgWvmZ.png" width="236"><img src="https://i.imgur.com/bR7Etyk.png" width="283">

The original design parameters were to display a room schedule electronically with no wires, at cheapest possible price point.  No Ethernet, no power wires, in a low-cost footprint.  The resulting project has produced a AA powered WiFi E-ink device for under $100 (US), where the batteries last a year or two.  


The wall-ink project along with the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) include everything necessary to build the entire e-ink display system 
 
# Summary
The wall-ink project has several components

* Circuit and electronics, including Eagle schematic, PCB layout and gerber files
* List of necessary parts to build the project
* Arduino IDE Firmware for the ESP8266 microcontroller
* CAD files to 3D print case
* See the [Wall-Ink-Server](https://github.com/caedm/wall-ink-server) project project for server side code

# Circuit
The wall-ink circuit contains a the following sections: the screen, the ESP8266 microprocessor, power regulation, deep-sleep wakeup, voltage monitoring, user-input, and firmware update circuitry.  See the [wiki](https://github.com/caedm/wall-ink/wiki/circuit) for a full explanation of this circuitry.
<img src="https://i.imgur.com/yiEXq9H.png" width="500">

# PCB layout
A two-sided PCB layout is part of the project including both the Eagle board and the gerber files.  The gerber files can simply be uploaded to a company to make the boards as seen here without any knowledge of PCB layout or design.  You will however, need to purchase all of [the parts](https://github.com/caedm/wall-ink/wiki/partslist)  and solder them to the board.  There are only two surface mount components, which are large enough to be hand soldered.

<img src="https://i.imgur.com/uxgWvmZ.png" width="300"><img src="https://i.imgur.com/cEBkq4L.png" width="360">

# Parts list
A full list of parts and suppliers can be found in the [wiki parts list](https://github.com/caedm/wall-ink/wiki/partslist).

# Firmware
Although wall-ink was originally designed as a delivery system for room schedules, it can be used to deliver any image to an e-ink screen.  When the wall-ink firmware [[boots]], it attaches to WiFi, gets a DHCP address, and then contacts the [[web server]] it has been configured to pull an [[image]] from.  It downloads an image, along with instructions of how long to [[sleep]] for.  It can sleep for a few seconds, or many hours.  The firmware was written in C using the Arduino IDE.

# CAD files
The "back" or "mounting bracket" of the unit was intended to be permanently affixed to a wall, or other mounting surface.  The unit itself slides into the mounting bracket and is held on by rails and tabs.  There are holes for security screws to stop curious individuals from lifting up on the device to remove it.  Both the front and back together are roughly over 100 grams of plastic.
