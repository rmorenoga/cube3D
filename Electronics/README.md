# Electronic PCB design files

This folder contains the design files for fabricating the PCBs that make the sides of the cube and that enable processing, power and communication. Each folder contains a [Kicad](kicad.org/) project with both schematics and layout files. 

## 3DESPWrover 
This is the top PCB that contains an ESP32-WROOVER module, a DC-DC step down converter and a Neo-pixel RGB led. The BoardTest subfolder contains test code to check that all components are working properly in the board. This PCB contains a 14-way micromatch connector to connect this and the bottom PCB together. One top PCB is required for one cube.

## 3DESPWroverS
This is the side PCB. This PCB contains only communication and power headers as well as soldering connections to attach it to other similar PCBs and the bottom one. Four identical side PCBs are required for one cube.

## 3DESPWroverB
This is the bottom PCB. This PCB contains only communication and power headers as well as soldering connections to attach it to the side PCBs. This PCB contains a 14-way micromatch connector to connect this and the top PCB together. One bottom PCB is required for one cube.