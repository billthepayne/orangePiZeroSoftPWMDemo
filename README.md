# orangePiZeroSoftPWMDemo
Found that the Orange Pi Zero was not accurately reflected in the platform-specific wiringPI library header files.  Patch file here should remedy that.  Also includes a test program to validate das blinkenlights.

Assumptions:
- You have an Orange Pi Zero (original model, not one of the groovy Plus versions)
- You've got an RGB LED hooked up to pins 8(B), 12(R), and 23(G).
- You want to see some blinkenlights, and some fake-PWM *faden*lights.

## Instructions

1) Get the wiringPI Orange-pi updates
```
git clone https://github.com/orangepi-xunlong/wiringOP.git
cd wiringOP
git checkout 569aed843bae56169259f3af12fe1a418b148b5e
```

2) Apply the patch for the original Orange pi model Zero (non-functional in the current branch)
```
patch -p2 < ../wiringOP_orange_pi_zero_offsets.patch
```

3) Build and install gpio command & associated libraries, ensuring correct platform:
```
PLATFORM=OrangePi_ZERO ./build
```

4) Build demo app - note, slight issue with timing on the sunxi platform...
```
cd ..
gcc -c -O3 -Wall -I/usr/local/include -Winline -pipe pwm-test1.c -o pwm-test1.o
```

5) Link demo app, create executable:
```
gcc -o pwm-test1 pwm-test1.o -L/usr/local/lib -lwiringPi -lwiringPiDev -lpthread -lm -lcrypt -lrt
```

6) Take it away:
```
./pwm-test1
```
