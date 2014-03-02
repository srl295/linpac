/*
   Bell command for LinPac
   (c) 1998 by Radek Burget (OK2JBG)

   This has been tested on i386 Linux only
   Please report functionality at other platforms
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/soundcard.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "lpapp.h"
#include "doorbl.h"

#define RATE 22025
int console;

//Procudes sound via PC-speaker
void sound(int frequency)
{
  unsigned long arg = 1193180/frequency;
  ioctl(console, KIOCSOUND, arg);
}

//Stops sound via PC-speaker
void nosound()
{
  unsigned long arg = 0;
  ioctl(console, KIOCSOUND, arg);
}

void wait(int micros)
{
  struct timeval tim, wtim;
  struct timezone tzon;
  
  gettimeofday(&tim, &tzon);
  wtim.tv_sec = tim.tv_sec;
  wtim.tv_usec = tim.tv_usec + micros;
  while (wtim.tv_usec > 1000000) { wtim.tv_usec=wtim.tv_usec-1000000; wtim.tv_sec++; }
  do 
    gettimeofday(&tim, &tzon); 
  while ((tim.tv_sec < wtim.tv_sec) ||
        ((tim.tv_sec == wtim.tv_sec) && (tim.tv_usec < wtim.tv_usec)));
}

int main(int argc, char **argv)
{
  int dsp;
  int i;
  int active;

  active = lp_start_appl();
  lp_event_handling_off();
  if (active) lp_appl_result("Ringing (PC speaker only)...");

  /*ki6zhd / dranch: disabled soundcard and using PC speaker only*/
  /* dsp = open("/dev/dsp", O_RDWR);
  if (dsp != -1) //soundcard present
  {
    i = AFMT_U8;
    ioctl(dsp, SNDCTL_DSP_SAMPLESIZE, &i);
    i = RATE;
    ioctl(dsp, SNDCTL_DSP_SPEED, &i);
    write(dsp, data, sizeof(data));
    close(dsp);
  }
  else //soundcard not present - use speaker
  {
    int i;
  */
    console = open("/dev/console", O_RDWR);
    if (console == -1) printf("\a"); //nothing usable, try BEL char.
    for (i=0; i < 10; i++)
    {
      sound(600); wait(20000);
      sound(800); wait(20000);
    }
    nosound();
  /* } //disabling of the soundcard */
  if (active) lp_end_appl();
  return 0;
}

