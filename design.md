# Design Notes:

## The Software

- The software may be confusing to someone just beginning their microcontroller
journey. The main method has no looping section! But, it turns out all of the 
looping routines are handled in interrupts. But first, let's look at the setup()
section, which sets many registers to their correct values.

  - First, the global interrupt switch is disabled with cli(). This is important, 
  since it is very bad if they start activating when they are still being set up!
  - DDRB, the Data Direction register, controls whether pins are inputs or outputs.
  Here, we set the SERCLK and SERDATA pins to outputs, so they can drive the shift
  register.
  - MCUCR, the Microcontroller Control register, controls how the external and pin 
  change interrupts are activated. We'll get to those later. We set the external
  interrupt INT0 to activate on a falling edge of pin2, the INT0 pin
  - PORTB is the Pin Port B register, which controls the digital value written to 
  the pins. When a pin is an output 1 is high and 0 is low; when it is an input, 1
  is pull-up and 0 is floating.
  - GIMSK, the General Interrupt Mask, controls whether the external and pin change
  interrupts are enabled. We'll get to those later. We enable the INT0 interrupt.
  - TIMSK, the Timer Interrupt Mask, controls whether various timer interrupts are
  enabled. We'll get to those later. We enable the Timer1 Compare and Timer0 
  Overflow interrupts.
  - PRR, the Power Reduction register, controls whether various peripherals are 
  turned off for power saving. We disable the Universal Serial Interface to save 
  a few nanoamps.
  - DIDR0, the Digital Input Buffer register, controls whether digital input is 
  turned off for some pins for power saving. We turn it off on the unnecessary pins
  to save some power.
  - TCCR0A/B, the Timer0 Control register A/B, configure Timer 0. Here we enable it 
  in Normal mode, and give it a prescaler of 1024 - which with the 16MHz sys clock 
  ticks the timer at 16kHz.
  
  - Now after all the interrupts have been properly set up, we can enable interrupts
  globally with sei().
  
  - It is important that no more than one shift register output is ever high, since 
  that could quickly overwhelm the current capability of the multiplexing hardware.
  So, using PORTB we strobe the SERCLK pin 8 times while holding SERDATA low to make
  sure that not a single flip flop inside holds a high output. Finally, we set the 
  inverted OE pin to output low, pulling that pin low and enabling the output of the
  shift register.
  
- And now, the main method will loop doing nothing, forever. The rest of the code is 
a couple of ISRs, or Interrupt Service Routines. What are those?
- An Interrupt is a way for microcontrollers to respond to stimuli in a real-time 
manner. Consider the task of reading a button, lighting up an LED whenever it is 
pressed and turning it off when it is not. One way to do this is to poll the button
many times per second, checking the state of the pin it is connected to and deciding
the LED's state based on that. The problem is that if this polling is done infrequently 
there is a good chance that the CPU will not notice that the button was pressed for a
while. Conversely, if polling is done frequently, this can waste a ton of CPU time.
- Probably a better way to handle this task is to assign interrupt routines.
If there is an interrupt routine that turns on the LED on a falling edge of the button
pin, and a routine that turns off the LED on a rising edge, then the CPU can do 
something else, receive an interrupt, put that other task away, handle the LED, then
go right back to that task seamlessly. Very little latency happens between the button
press and the LED light, and the CPU wastes very little time constantly checking for 
the button.
- Interrupts on the ATTINYX5 can be activated through numerous vectors, including 
RESET, INT0 external interrupt, PCINT0 pin change interrupt, TIMER0/1_COMPA/B timer
compare match, TIMER0/1_OVF timer overflow, ADC conversion complete, and more. Note
that some of these can be caused by the microcontroller itself, which can be very useful
when setting up timed events that should not depend on the CPU to time them.
- Finally, let's look at the first of our interrupt service routines, or ISRs.

  - ISR(TIMER0_OVF_vect) is the ISR that activates when timer0 overflows. The setup 
  section configured timer0 to count up to 255 and then overflow back to 0, endlessly,
  with timer ticks happening at 16kHz. This means that a timer overflow will happen at
  64Hz, so this routine pretty much unconditionally fires every 1/64th of a second.
  
    - The ADC reading of the reset pin is taken with analogRead(A0). You could eliminate
    this Arduino function call and replace it with ADC register changes, explained in my
    tiny-simon project, but that is not necessary. 
