# Inductive Metal Detector

A 3-coil inductive metal detector built on a BASYS 3 FPGA board using a 
MicroBlaze soft-core processor programmed in embedded C.

## How It Works
Three inductive coils (left, center, right) continuously read analog voltage 
signals via ADC channels. When metal is detected near a coil, the voltage drops 
below a preset threshold, triggering a 1-second debounce window to prevent false 
repeated detections.

## Features
- Real-time proximity strength displayed across 16 LEDs, scaled to the voltage 
  drop magnitude
- Individual detection counts per coil tracked and displayed on a 
  seven-segment display
- 3-coil coverage for left, center, and right detection zones

## Tools & Languages
- Embedded C
- Xilinx Vitis / Vivado
- BASYS 3 FPGA Board (MicroBlaze)
