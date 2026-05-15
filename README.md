# Elektron Team - WRO Future Engineers 2026 - Bosnia and Herzegovina

## Content
* `Team Photos` contains 2 photos of the team (an official one and one funny photo with all team members)
* `Robot Photos` contains 6 photos of the vehicle (from every side, from top and bottom)
* `Videos` contains the video.md file with the link to a video where driving demonstration exists
* `Schematics` contains schematic diagrams of the driving base.
* `Code` contains code of control software for all components which were programmed to participate in the competition.
* `3D Models` is for the files for models used by 3D printers to produce the vehicle elements.
* `Misc` is for other files which can be used to understand how to prepare the vehicle for the competition.

## Introduction

This is the public repo for our team and the work we have done on our robot.

### Robot Gallery

<p float="left">
  <img src="Robot Photos/right.jpg" width="350"/>
  &nbsp;
  <img src="Robot Photos/Pics/servo-and-tof.jpg" width="350"/>
</p>

## Technical & Mechanical Specifications

### Mechanics & Drive
- Fully custom built and 3d printed robot base
- High torque, medium speed driving system
- Servo steering system

### Electronics
- 5Ah 2S battery pack (Li-Po)
- Time-of-Flight distance sensors
- Raspberry Pi 5 8GB SBC
- Raspberry Pi Pico 2 Zero MCU
- Full-axis IMU sensor
- Addressable LED indicators
- 2.4in TFT Touch Control LCD

### Size
- 205x135x65mm (LxWxH) without camera mount, 205x135x~150mm (LxWxH) with camera mount
- ~490g total weight

<p float="left">
  <img src="Robot Photos/Pics/length.png" width="150"/>
  &nbsp;
  <img src="Robot Photos/Pics/width.jpg" width="300"/>
  &nbsp;
  <img src="Robot Photos/Pics/weight.jpg" width="280"/>
</p>



## How does it work?

### Driving system
The main driving motors are 2 N30 motors in a 1:150 configuration, then the power is transferred to the wheels with a 3:1 gear ratio, the motors feature hall effect sensors. 

<p float="left">
  <img src="Robot Photos/Pics/driving-base.jpg" width="400"/>
  &nbsp;
  <img src="Robot Photos/Pics/driving-base-3dprint.jpg" width="400"/>
</p>

We have tested a couple different configurations (mainly 1:3 and 1:1) out of which, 3:1 turned out to work the best for our needs as these motors spin quite fast but are quite low torque. For extra torque, we decided to include 2 motors instead of one. They are connected to the DRV8833 H-Bridge IC on separate channels **BUT** the motors are connected onto one shared shaft together. The separte channels are exclusively to distribute load evenly on the IC.

We are using a 110mm long and 3mm in diameter steel shaft and plastic gears.

Wheels are 3D printed, each wheel being 30mm in diameter without the outer rubber.

<p float="left">
  <img src="Robot Photos/Pics/wheel.png" width="350"/>
  &nbsp;
</p>

These specific motors have 2 hall effect sensors each, one for speed and one for direction, however we are only using one set from one motor, the other one stays disconnected and should only be used in case that something goes wrong with the other motor.

Speed control is done via a PID algorithm handled by the Pi Pico 2.

The motors are controlled by a DRV8833 IC which is current-limited to 1A per channel.

<p float="left">
  <img src="Robot Photos/Pics/drv8833.jpg" width="350"/>
  &nbsp;
  <img src="Robot Photos/Pics/driving-mech-assembled.jpg" width="500"/>
</p>

### Steering system
The steering system is controlled by one main MG90S servo from Waveshare.

Last year, we had issues with our steering system on the Open Championship in Ljubljana which was caused by a fake "metal" servo. Hence why we opted for higher quality full metal gear servos, and the MG90S was a great option as we didn't really need more power than that.

Steering power is transferred to the wheels with a steering shaft and poles on which we tied some rubber bands to help center the steering mechanism.
With this configuration, we are able to achieve a steering angle of 50 degrees which fits our needs nicely.

<p float="left">
  <img src="Robot Photos/Pics/steering-base.jpg" width="400"/>
  &nbsp;
  <img src="Robot Photos/Pics/servo-and-tof.jpg" width="400"/>
</p>

### Power and power delivery
We had originally thought about using a 3S battery system like we did last year, however, we switched to a 2S system now to save on space and weight. With some clever engineering tricks and overcomplication the power delivery side, that was super easy to do.

<p float="left">
  <img src="Robot Photos/Pics/initial-assembly.jpeg" width="350"/>
  &nbsp;
</p>

The main power source are 2 5360105 Li-Po 3.7v 5Ah cells connected in series, they each have their own integrated BMS for safety.
We have decided to continue to charge our cells with a USB-C charger, but this year we have switched to a much more efficient IP2326 module for charging (and also because our previous battery had a dedicated BMS as it was built from bare 18650 cells and had balancing included which the 2 separate battery cells don't).

The actual power delivery part is split up into a couple different stages, one buck regulator for the Pi 5 exclusively and 2 smaller LDO regulators for the Pi Pico 2, sensors and another for the MG90S servo.

The main power regulator IC is a TPS54623RHLR. It's a pretty recent chip with not so much documentation available from other hobbyists but due to its very attractive price and great performance claims, we had to give it a shot. After designing a circuit that works for our needs in Ti's WEBENCH design utility (which are basically 5V output and at least 5A output max), we had ourselves a very nice and reliable regulator config. 

Robots are noisy machines, both literally in an audible sense, and power wise. Last year we had major issues with premade modules for power regulations which just couldn't handle our Pi, so this year, we decided to go overkill on ensuring our robot has clean power everywhere at all times. Our decoupling circuit includes:
  -  One 220uF 10V 10TPE220ML Panasonic Polymer Tantalum capacitor for bulk power storage.
  -  Two 10uF 6.3V X6S CL10X106MQ8NNNC Ceramic MLCC Samsung capacitors for mid frequency decoupling
  -  Four 100nF 6.3V X7R CGA0402X7R104K6R3GT Ceramic MLCC HRE capacitors for high frequency decoupling which were placed right next to the GPIO power pins of the Pi 5

<p float="left">
  <img src="Robot Photos/Pics/5v-reg-1.png" width="400"/>
  &nbsp;
  <img src="Robot Photos/Pics/vreg.jpg" width="400"/>
</p>

For the other two power regulators, we decided to go with the following combination:
  - AP2112K-3.3TRG1 3.3V 1A LDO regulator for the Pico 2 and 3.3V sensors (ToF and IMU)
  - ME6118A50PG 5V 1A LDO regulator for our MG90S servo

<p float="left">
  <img src="Robot Photos/Pics/5v-3v3-reg-1.png" width="400"/>
  &nbsp;
</p>

### The Brain
Our robot runs on a Raspberry Pi 5 (8GB). We chose this SBC because we had it available and because the only other option we had was a Pi 4 with 2GB of ram. The choice is obvious :D
The Pi 5 communicates with the Pi Pico 2 Zero through UART on pins GP14/15 (and pins GP0/1 on the Pico 2). 

Workloads are distributed between both the SBC and the MCU like this:
<p float="left">
  <img src="Robot Photos/Pics/comms-topology.png" width="350"/>
  &nbsp;
</p>

Pi 5 Handles camera input and OpenCV image processing and then sends movement data to the Pi Pico 2 Zero. 

We use a simple UART protocol to communicate between the SBC and MCU:
- Every packet
  
  `[0xAA] [TYPE] [LEN] [PAYLOAD...] [CRC8]`
  - `0xAA` Start byte, always
  - `TYPE` What kind of message
  - `LEN` Payload length in bytes
  - `CRC8` Checksum of everything except start byte

- Pi 5 -> Pico 2 Zero (commands): `TYPE = 0x01`
  
  `[0xAA] [0x01] [04] [dir] [speed] [servo] [led_mode] [CRC8]`
  - `dir` 0 = stop, 1 = fwd, 2 = bwd
  - `speed` 0 - 255
  - `servo` 0 - 180
  - `led_mode` 0 = idle, 1 = driving, 2 = obstacle, 3 = low battery

- Pico 2 Zero -> Pi 5 (telemetry): `TYPE = 0x02`
  
  `[0xAA] [0x02] [14] [tof0_H] [tof0_L] [tof1_H] [tof1_L] [tof2_H] [tof2_L] [tof3_H] [tof3_L] [batt_H] [batt_L] [yaw_H] [yaw_L] [enc_H] [enc_L] [CRC8]`
  - ToF values in mm, split into hi/lo bytes
  - Battery in mV
  - Yaw in tenths of a degree (so 1234 = 123.4 degrees)
  - Encoder ticks as int16

The robot is controlled through the 2.4in TFT Touchscren LCD display. It also shows all of the telemetry data collected from the Pi Pico 2 Zero and allows for manual control when debugging. The competition-ready interface will just feature a simple option to select between running the obstacle or open challenge code and one start button. 


### Sensors
After running into a lot of trouble with ultrasonic distance sensors (hc-sr04) last year, we now use Time-of-Flight distance sensors. The most readily available sensor of this kind on the market is the VL53Lx series of sensors.

We use 3 ToF sensors on the robot with the option of adding a 4th sensor (rear side sensor) if needed. The data is polled in single shot mode on each sensor for higher speed and precision.

<p float="left">
  <img src="Robot Photos/Pics/vl53l0x.jpg" width="350"/>
  &nbsp;
</p>

For getting the turn angle we use the MPU6050 IMU sensor. It's not the most accurate sensor and has a little bit of drift when idle but that's nothing resetting the sensor yaw angle before every turn cant fix. 

<p float="left">
  <img src="Robot Photos/Pics/mpu6050.png" width="350"/>
  &nbsp;
</p>



