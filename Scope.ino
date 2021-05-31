//SET THE ATTINY85 to 16MHz operation!

void setup() {
  // put your setup code here, to run once:
  cli(); //global interrupt disable
  
  DDRB = 0b00000011; //pin0:SERCLK pin1:SERDATA outputs

  MCUCR |= 0b00000010; //INT0 interrupt: falling edge detect 
  MCUCR &= 0b11111110; //INT0 interrupt: falling edge detect 
  
  PORTB |= 0b00010100; //Pullup pin2:TriggerInput pin4:TriggerMode 
  GIMSK |= 0b01000000; //INT0 vector enable
  
  TIMSK |= 0b01000010; //TIMER1_COMPA and TIMER0_OVF vectors enable

  PRR = 0b00000010; //disable USI for power saving

  DIDR0 = 0b00101011; //disable unnecessary digital inputs 0, 1, 3, 5

  //flush out the shift register without using SERCLR by toggling SERCLK 8 times
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  PORTB = 0b00000001;
  PORTB = 0b00000000;
  
  DDRB |= 0b00001000; //pin3:!OE output low to enable output

  TCCR0A = 0b00000000; //Timer0 in Normal mode
  TCCR0B = 0b00000101; //Timer0 prescaler of 1024, 16kHz clock ticks, or 64Hz Timer0 Overflows
  
  sei(); //global interrupt enable

  for(;;){} //loop nothing forever
}

ISR(TIMER0_OVF_vect){ //sets the sweep frequency every 16ms or so, Timer0 Overflow
  volatile uint16_t reading = analogRead(A0);
  volatile uint16_t denoised = (denoised - reading > 8 || reading - denoised > 8)? reading : denoised; 
  volatile uint16_t constrained = (denoised < 514)? 514 : denoised; //Force into one of 15 * 34 = 510 values

  const static uint8_t ocr[] PROGMEM = {251, 246, 241, 236, 231, 227, 222, 217, 213,
                                        209, 205, 200, 196, 192, 189, 185, 181, 
                                        177, 174, 170, 167, 163, 160, 157, 153, 151,
                                        148, 145, 142, 139, 136, 133, 131, 128}; //34 entries in geometric sequence

  uint8_t prescaler = 15 - (constrained - 514) / 34; 
  //timer1 prescaler should range from 0b1111 to 0b1, 0b1 being fastest (/2) and 0b1111 slowest (/16K)
  
  OCR1A = OCR1C = pgm_read_byte_near(ocr + (constrained - 514) % 34); //set OCR1C to correct value based on table for CTC 
  //do the same to OCR1A for the compare match interrupt
    
  TCCR1 = 0b10000000 | prescaler; //enable timer in CTC mode and set prescaler
}

volatile uint8_t serTimer = 7; //shifts remaining to flush registers, decrements with ISR

ISR(TIMER1_COMPA_vect){ //fires the sweep when trigger mode is off, and sweeps the display
    if (!serTimer && (PINB & 0b00010000)){ //only fires when pin4 high and shift register empty
    PORTB |= 0b00000010; //SERDATA high
    serTimer = 7;
  }
    PORTB |= 0b00000001; //SERCLK high
    PORTB &= 0b11111100; //SERCLK SERDATA low

    if (serTimer) serTimer--; //countdown
}

ISR(INT0_vect){ //fires the sweep when trigger mode is on
  if (!serTimer && !(PINB & 0b00010000)){ //only fires when pin4 low and shift register empty
    PORTB |= 0b00000010; //SERDATA high
    PORTB |= 0b00000001; //SERCLK high
    PORTB &= 0b11111100; //SERCLK SERDATA low
    serTimer = 6;
    TCNT1 = 0; //reset the timer to allow for the correcct time before the next serclk tick
  }
}
