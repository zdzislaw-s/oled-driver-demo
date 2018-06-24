# OLED Driver Demo

## Overview
This repository includes files that are required for building a demo application that is presented in the [this video clip](https://github.com/zdzislaw-s/oled-driver-demo/blob/master/oled-driver-demo.mp4). The schematic diagram for the hardware part of the project is presented in [this .pdf file](https://github.com/zdzislaw-s/oled-driver-demo/blob/master/oled-driver-demo.pdf).

## Hardware
- ZedBoard Rev. D

## Software
- Ubuntu 16.04 LTS
- Vivado 2018.1

## Instructions for Building and Running
1. After cloning the repository, run the following command in a terminal:
    ```
    $ vivado -mode batch -source create-oled-driver-demo.tcl
    ```
2. Start Vivado in GUI mode and open the `oled-driver-demo/oled-driver-demo.xpr` project file.
3. Generate the bitstream.
4. Export the generated hardware (including the bitstream).
5. Launch the SDK.
6. In the SDK, create an application project with freertos as the OS platform and C++ as the language.
7. Setup the created application project in such a way that it includes `oled-driver-demo-freertos.cpp` and `Ssd1306.hpp` files.
8. Setup the include directories accordingly (make sure that `ps/resources` is in the list of those directories).
9. Build, Program FPGA and Launch on Hardware.

That's all.

### Acknowledgements
In working on this project, I found the following resources, to greater or lesser extent,  helpful:
- (https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- (https://cdn-shop.adafruit.com/datasheets/UG-2832HSWEG04.pdf)
- (https://github.com/jmwilson/Adafruit_SSD1306_MicroBlaze)
- (https://github.com/Digilent/Zedboard-OLED)
- (http://minized.org/zh-hant/comment/1766#comment-1766)
