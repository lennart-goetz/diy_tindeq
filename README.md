When training for climbing, you will come across some specialzied training tools. 

Some measure the force the climber can pull on a ledge, and visualize them on a phone. 
Those devices are quite expensive, so this is my try to build my own. 

This project is based on a crane-scale to get the strain gauge. 
To measure the small Voltage across it, the HX711 IC is used. To visualize and process
the data, I use a ESP32 S3. I went for S3 because it can be programmed directly via usb, without the need for additional usb-serial adapters. 

In the current Version (V1) There are 4 pads on the left, that are connected to the strain gauge. 
On the right side, there are 4 pads for power supply. V-bus and b-gnd are 5V and gnd from the usb port. They will be connected to external Battery management system. The output of the external Battery mangement System will be connected to OUT+ and OUT- 

The pcb is designed to support charging via USB and programming via usb. 
I am using a TP4056 Module for Managing the single cell Lithium batterie. Make sure your BMS support overdischarge protection and overcharging protection. 

The pcb is not tested yet, I will probably continue updating this repo after testing. 
