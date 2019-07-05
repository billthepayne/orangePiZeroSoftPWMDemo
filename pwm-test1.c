//
//  Low-level GPIO manipulation for RGP LED
//  + threading for manual PWM / freq control on each pin
//
//  LnxProf
//
//  Version 0.2
//    Date: 2019-June-29
//    Notes: Initial version - validating functionality
//
//  Changes:
//    v0.2: Modified to work on the OrangePi (note, library for opiZero has incorrect offsets... be sure to use patch from here:)
//          https://github.com/billthepayne/orangePiZeroSoftPWMDemo
//
//  Assumptions:
//    R: GPIO_12
//    G: GPIO_23
//    B: GPIO_8

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <wiringPi.h>



struct gpioThreadData {
  short id;
  short GPIONUM;
  short intensity;
  short keepRunning;
};

/***************************************************
* -- Constants --
*       ... Modify with caution
*
*    Reducing targetFreq will increase granularity of fades, however, blinkiness MAY ensue.
***************************************************/
// mindelay and ucostleep were .0001
float minDelay = 0.0001;
short targetFreq = 100; // Was 250 -
float period;
//float period = (float)(1 / (float)targetFreq);
float usleepCost = 0.0001; // over-estimate - actually about 0.000075
float steps;
//float steps = 1 / (float)targetFreq / usleepCost;


void setup_io();

// Function to manage a single color of the RGB spectrum
// Should be passed a struct containing GPIO number and intensity
// *** This should be created as a unique memory location in main so that we can update intensity on the fly
void * ledThread(void *_threadData) {
  // Variables... for... things.
  struct timespec startDuty;
  struct timespec currTime;
  float targetDuration;
  short intensity; // 0 - 255
  short ledOn = 0; // 0 or 1
  time_t startEpoch = time(NULL);
  float scaledIntensity; // Scaled from 8-bit intensity value

  // This is totally thread safe... And awesome.
  //       * Said nobody, anywhere...
  struct gpioThreadData *threadData = (struct gpioThreadData *) _threadData;
  short LEDGPIO = threadData->GPIONUM;

  clock_gettime(CLOCK_MONOTONIC, &startDuty); // Initialize to current time

  wiringPiSetup ();
  pinMode (LEDGPIO, OUTPUT);

  while (threadData->keepRunning) { // Run until asked nicely to stop
    intensity = threadData->intensity;  // Hell yeah!
    if (intensity > 255) {
      intensity = 255;
    } else if (intensity < 0) {
      intensity = 0;
    }
    if (intensity > 0 && intensity < 255) { // Only calculate if we need PWM, otherwise set GPIO and usleep for period
      scaledIntensity = (float)((float)intensity / (float)256.0); // Calculated each round to catch changes..
                                                 // TODO: Is this less expensive than tracking & comparing changed var?
      if (ledOn == 0) {
        digitalWrite(LEDGPIO, LOW);
        ledOn = 1;
        targetDuration = (float)((float)scaledIntensity * (float)period);
      } else {
        digitalWrite(LEDGPIO, HIGH);
        ledOn = 0;
        targetDuration = (float)((1.0-(float)scaledIntensity) * (float)period);
      }
      if (((targetDuration - usleepCost) * 1000000) > 0) {
        usleep((targetDuration - usleepCost) * 1000000);
      } else {
        usleep(1); // Actually ends up being something like 80usec... stupid scheduler.
      }
    } else {
      if (intensity == 0) {
        digitalWrite(LEDGPIO, HIGH);
      } else {
        digitalWrite(LEDGPIO, LOW);
      }
      usleep((int)((period - usleepCost) * 1000000)); // sleep for this period
    }
  }

  printf("Thrad #%d Exiting.  Ran for (%d) seconds.\n", threadData->id, (time(NULL) - startEpoch));
  free(threadData);
  pthread_exit(NULL);
} // ledThread

int main(int argc, char **argv)
{
  pthread_t tid1;
  pthread_t tid2;
  pthread_t tid3;

  struct gpioThreadData *t1Data = malloc (sizeof (struct gpioThreadData));
  struct gpioThreadData *t2Data = malloc (sizeof (struct gpioThreadData));
  struct gpioThreadData *t3Data = malloc (sizeof (struct gpioThreadData));
  period = (float)(1 / (float)targetFreq);
  steps = 1 / (float)targetFreq / usleepCost;

  t1Data->id = 1;
  t2Data->id = 2;
  t3Data->id = 3;
  t1Data->GPIONUM = 12;
  t2Data->GPIONUM = 23;
  t3Data->GPIONUM = 8;
  t1Data->intensity = 1;
  t2Data->intensity = 1;
  t3Data->intensity = 1;
  t1Data->keepRunning = 1;
  t2Data->keepRunning = 1;
  t3Data->keepRunning = 1;

  // Set up gpi pointer for direct register access
  setup_io();

  // Switch GPIO 12(R), 23(G), & 8(B) to output mode
  // TODO: Allow setting of GPIO values during itialization
  //INP_GPIO(10); OUT_GPIO(10); // must use INP_GPIO before we can use OUT_GPIO
  //INP_GPIO(13); OUT_GPIO(13); // must use INP_GPIO before we can use OUT_GPIO
  //INP_GPIO(2); OUT_GPIO(2); // must use INP_GPIO before we can use OUT_GPIO


  pthread_create(&tid1, NULL, ledThread, t1Data);
  pthread_create(&tid2, NULL, ledThread, t2Data);
  pthread_create(&tid3, NULL, ledThread, t3Data);


  int defSleep = 333333;
  // Threads created... let's dance.
  sleep(4);  // All at lowest possible intensity to start.

  // One second for each, R, G, B;
  printf("Red..\n");
  t1Data->intensity = 255;
  t2Data->intensity = 0;
  t3Data->intensity = 0;
  usleep(defSleep);
  sleep(1);
  printf("Green..\n");
  t1Data->intensity = 0;
  t2Data->intensity = 255;
  t3Data->intensity = 0;
  usleep(defSleep);
  sleep(1);
  printf("Blue..\n");
  t1Data->intensity = 0;
  t2Data->intensity = 0;
  t3Data->intensity = 255;
  usleep(defSleep);
  sleep(1);

  // Anyone for mixed drinks?
  t1Data->intensity = 255;
  t2Data->intensity = 127;
  t3Data->intensity = 0;
  usleep(defSleep);
  t1Data->intensity = 0;
  t2Data->intensity = 255;
  t3Data->intensity = 255;
  usleep(defSleep);
  t1Data->intensity = 255;
  t2Data->intensity = 0;
  t3Data->intensity = 255;
  usleep(defSleep);

  // Bright white(ish):
  t1Data->intensity = 0;
  t2Data->intensity = 0;
  t3Data->intensity = 0;
  usleep(defSleep);

  // fade 1 colo
  int i,j;
  for (j = 0; j < 20; j++) {
    for (i = 0; i <= 255; i+=1+(int)(i/50)+j) {
      t1Data->intensity = i;
      t2Data->intensity = 0;
      t3Data->intensity = 0;
      usleep(10000);
    }
    for (i = 255; i >= 0; i-=(1+(int)(i/50)+j)) {
      t1Data->intensity = i;
      t2Data->intensity = 0;
      t3Data->intensity = 0;
      usleep(10000);
    }
  }
  // fade 1 colo
  for (j = 0; j < 20; j++) {
    for (i = 0; i <= 255; i+=1+(int)(i/50)+j) {
      t1Data->intensity = 0;
      t2Data->intensity = i;
      t3Data->intensity = 0;
      usleep(10000);
    }
    for (i = 255; i >= 0; i-=(1+(int)(i/50)+j)) {
      t1Data->intensity = 0;
      t2Data->intensity = i;
      t3Data->intensity = 0;
      usleep(10000);
    }
  }
  // fade 1 colo
  for (j = 0; j < 20; j++) {
    for (i = 0; i <= 255; i+=1+(int)(i/50)+j) {
      t1Data->intensity = 0;
      t2Data->intensity = 0;
      t3Data->intensity = i;
      usleep(10000);
    }
    for (i = 255; i >= 0; i-=(1+(int)(i/50)+j)) {
      t1Data->intensity = 0;
      t2Data->intensity = 0;
      t3Data->intensity = i;
      usleep(10000);
    }
  }
  // fade
  for (j = 0; j < 20; j++) {
    for (i = 0; i <= 255; i+=1+(int)(i/50)+j) {
      t1Data->intensity = i;
      t2Data->intensity = i;
      t3Data->intensity = i;
      usleep(10000);
    }
    for (i = 255; i >= 0; i-=(1+(int)(i/50)+j)) {
      t1Data->intensity = i;
      t2Data->intensity = i;
      t3Data->intensity = i;
      usleep(10000);
    }
  }


  // Signal threads that it's time to die...
  t1Data->keepRunning = 0;
  t2Data->keepRunning = 0;
  t3Data->keepRunning = 0;

  // Wait... TODO: less waiting, more killing.
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  pthread_join(tid3, NULL);

  // Set all LEDs to off.
  digitalWrite(8, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(23, HIGH);

  return 0;

} // main


//
// Set up a memory regions to access GPIO
//
void setup_io()
{
  wiringPiSetup ();

  pinMode (8, OUTPUT);
  pinMode (12, OUTPUT);
  pinMode (23, OUTPUT);
} // setup_io