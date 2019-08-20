## PSOC_EBC3.1_WS

Tested build environment:
```Windows 10 64-bits #PSOC Creator 4.2```

# Hardware

- ECB v3.1 board: Version3 rev 1 of the Embedded Controller Board(ECB), a small form factor swiss army knife for bio-robotics applications.
- MAT6 board: ECB for MAT6 6 legged robo with force sensing, and BNO085 IMU integration on the feet. 
- CY8CKIT-002 PSoC® MiniProg3 Program and Debug Kit.
- MicroUSB cable

# Software
- C programming Language.
- PSoC Creator Schematic GUI

# Projects

- P00_Blink_LED: Basic IO toggle test
- P01_Bootloader: Bootloader project for the ECBv3 board, please link any biotloadable project the bin/hex file generated to this folder.
- P02_Blink_LED_Bootloadable: Test project of using USBUART based bootloader to flash the ECB without MiniProg.
- P03_USBUART_Control_LED: Basic test for bi-directional USBUART communication. Using keyboard inputs, LED RGB values will adjust to new values.
- P04_a_CAN_TX_Test
- P04b_b_CAN_RX_Test
-P05_IMU_BNO085_Test: Original adaptation of IMU code from PSoC4 to PSoC5LP. Non working.
-P06_IMU_BNO085_WIP: Work in progress adaptation of IMU code from PSoC4 to PSoC5LP with I2C communication between PSoC and IMU. Successful implementation and configurable outputs for biorobotics applications. Current data output mode: Outputs 6 axis rotation vector, linear accelration, and gyroscope values at ~200 Hz, in quaternion, linear acceleration (xyz), and angular velocity (wx,wy,wz) format respectively.
-P07_IMU_BNO085_SPI: SPI implementation of P06, goal is to obtain ~6x faster output with SPI communication between IMU and PSoC. Not yet working! and ECBv3 Board needs more hardware modifications like reset and interrupt pins, along with slave select and other SPI specific pins pulled to the right polarity.
-P08_IMU_BNO085_MAT6: Adaptation of IMU code to another ECB, this time with a different PSoC processor. Hardware pins are changed and performs same functionality as P06.
-P09_MAT6_Bootloader: Bootloader project for MAT6 ECB.

P06 Project Details.

# Quick Start
- Using the P06 Code, one can successfully program the ECBv3 board to output three sensor fields: Orientation, linear acceleration, angular velocity in the form of 10 floats at a rate of 200 Hz from the PSoC. 

1. Setup
	a. Obtain code by pulling from this remote master. Folder should contain a PSoC Creator workspace with projects specified above. 
	b. Once the workspace is open, you will find a directory of projects on the left hand side of the IDE. Right click on P06_IMUBNO085_WIP and select `Set as Active Project`.
	c. Within `main.c`, configure the use modes for the board by commenting or uncommenting the following flags.
		- `#define USBUART_MODE` is used anytime there is a direct USB serial connection from the board to an external device, in our case, a CPU. Note: that when USBUART_MODE is defined, **debugging cannot be used**. If you debug with this mode activated, the code will hang forever in a while loop waiting for a USB port to be enumerated which can't happen during debugging. Use USBUART_MODE for printing out information whether that is for its primary purpose of outputting IMU data as floats, or outputting strings.
		- '#define DATA_OUTPUT_MODE` indicates that the PSoC is outputting IMU data as **floats** rather than strings. In this mode, the `printOut()` function is disabled, meaning there are no string outputs from the PSoC. This is the intended mode of use for the Blaser or MAT6 projects where the 10 floats of IMU information are needed through USB serial connection. **An important note:** In order for the PSoC to successfully output IMU float data, **both USBUART_MODE and DATA_OUTPUT_MODE need to be active.** Solely DATA_OUTPUT_MODE activated will result in no USB serial capability by the PSoC, which is useful in debug mode when one wants to see the procedure of outputting float data step by step.
		- If DATA_OUTPUT_MODE is undefined, there is an implicit mode of outputting strings. In this mode, printOut() function will work and can print any helper messages to the serial output. Like with DATA_OUTPUT_MODE, **this mode requires USBUART_MODE to be able to serially output strings to terminal.**

2. Program
	a. After configuring the outputs, the board must be programmed. The method of programming is determined by the hardware in two possible configurations. The first is traditionally with the miniprog3 programmer/debugger through SWD connection. The second is through bootloading the corresponding application/project onto the board. Check the `TopDesign.cysch` file for a Bootloadable component. If it is enabled, then bootloader is configured, which should be as default for this project.
	b. If the bootloadable is enabled, plug the board directly to the computer with USB and open the bootloader host under <Tools> in the taskbar. Further instructions to using the bootloader host can be found in the [Bootloader User Guide]((https://docs.google.com/document/d/1NsbHpMEDuHHZEE9elAJRFjD2x9ydBso8VCzAN2paOsE/edit).
	c. If bootloadable is not enabled, use the miniprog3 and select program under <Debug> in the taskbar. You may need to select the device, port acquire, and then program. 

3. Use
	If the bootloadable is enabled, there will be a brief ~3 Second bootloader program being run indicated by a rapidly blinking blue LED. Once the bootloader program has finished, the IMU application will run and output according to the configured output modes from the Setup.
	While the program is running there are serveral LED indicators for the status of the IMU.
	a. At the beginning of the IMU application, the IMU is initializing and setting up a USB connection. While the board is doing this it periodically activates the red LED in addition to the blue and green LEDs. The RGB LED will look like it is blinking between red and orange. 
	b. As soon as a USB serial connection is established with an external computer, the Red LED will cease to activate leaving only the blue and green LEDs to link periodically. 
	c. After a certain period of time, the Red LED should begin to periodically activate again, meaning the IMU has been calibrated onboard the PCB. If the Red light has not lit up after the initialization, it means the IMU is still dynamically calibrating  and the environment may not be constant enough for the IMU. This will result in less accurate outputs.
	**Note:** If the bootloadable is not enabled, the IMU will proceed directly to the IMU initialization stage, skipping the blue LED blink stage.

	To view the outputs, a script needs to be created to parse the 10 output floats in the following encoding: [float i, float j, float k, float r, float x, float y, float z, float wx, float wy, float wz]. The first four floats are for a quaternion for orientation, the next three floats are linear acceleration in x, y, z axes, and the last three floats are angular velocities along x, y, z axes.

# IMU Code Full Procedure Description
- Initialization
- Servicing
- Printing Data

# Useful Links
- [BNO085 IMU Datasheet](https://www.hillcrestlabs.com/downloads/bno080-datasheet)
- [SH2 Datasheet](https://cdn.sparkfun.com/assets/4/d/9/3/8/SH-2-Reference-Manual-v1.2.pdf)
- [SH2 specifics of SHTP Datasheet](https://www.hillcrestlabs.com/downloads/sh-2-shtp-reference-manual)
- [SHTP Datasheet](https://cdn.sparkfun.com/assets/7/6/9/3/c/Sensor-Hub-Transport-Protocol-v1.7.pdf)
- [STM BNO085 Implementation](https://github.com/hcrest/sh2-demo-nucleo)
- [Arduino BNO085 Implementation](https://github.com/sparkfun/Qwiic_IMU_BNO080/blob/master/Firmware/Tester/Tester.ino)
- [Bootloader and Bootloadable User Guide](https://docs.google.com/document/d/1NsbHpMEDuHHZEE9elAJRFjD2x9ydBso8VCzAN2paOsE/edit)
- [Miniprog 3 User Guide](https://www.cypress.com/file/44091/download)


