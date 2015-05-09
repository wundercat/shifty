
/*
   Data structure representing a moving dot
 */
struct dotz {
  // bit patterns
  short initialSprite[Nd];
  short sprite[Nd];
  short previousSprite[Nd];
  short mask[Nd];
  short stallMask[Nd];
  //durations
  short initialTickPeriod[Nd];
  short currentTickPeriod[Nd];
  short finalTickPeriod[Nd];
  short blankAfter[Nd];
  short stallDuration[Nd];
  //timers
  short jumpTimer[Nd];
  short stallTimer[Nd];
  short onTimer[Nd];
};
typedef struct dotz DOTZ;

#define SR "shift register"
/*
   Hardware definitions
    SER_ICK advances the SR. SER_OCK samples SR into output register
    Each register samples data at the rising edge of its clock.
    The two 8-bit SRs share all inputs except D.
 */

#define T5 TPIC6A595    // shift reg chip with 250 ma sink

/*
  Clear Bit Individually and Set Bit Individually are AVR opcodes.
  They're faster and smaller than the Arduino library routines,
  because they're not trying to be AVR model-agnostic.
 */

/*   Port B  */
#define UnoLED13  0x20  // Arduino D13 (PB5)
#define RunStop   0x02  // Arduino D9  (PB1)
#define SER_ICK   0x01  // Arduino D8  (PB0)
#define PBDIRR    UnoLED13 | SER_ICK
#define DDRB      0x04  // Port B direction control register i/o address

/*  Port D  */
/*  data to sr      Port D bit 7 => Arduino D7  */
#define SER_OUTD  0x80  // Arduino D7  (PD7)
#define deeHigh()    asm volatile(  "sbi 0x0b,7 \n\t" );
#define deeLow()     asm volatile(  "cbi 0x0b,7 \n\t" );

/*  data to sr      Port D bit 6 => Arduino D6  */
#define SER_OUTD2 0x40  // Arduino D6  (PD6)
#define dee8High()    asm volatile(  "sbi 0x0b,6 \n\t" );
#define dee8Low()     asm volatile(  "cbi 0x0b,6 \n\t" );

#define SER_OE_   0x08  // Arduino D3  (PD3)
#define SER_OCK   0x04  // Arduino D2  (PD2)
#define PDDIRR    		SER_OUTD | SER_OUTD2 | SER_OE_ | SER_OCK
#define DDRD      0x0a  // Port GD direction control register i/o address

/* Port A */
/*
  Analog input pin ranges 0 to 5V
 */
#define rheostatWiper  A0

/*  pulse srclk     Port B bit 0 => Arduino D8  */
#define srclkpulse() asm volatile(  "sbi 0x05,0 \n\t"  "cbi 0x05,0 \n\t");
/*  pulse outputreg Port D bit 2 => Arduino D2  */
#define clockitout() asm volatile(  "sbi 0x0b,2 \n\t"  "cbi 0x0b,2 \n\t");

/* SR "G_" pin, low true output enable.  Port D bit 3 => Arduino D3  */
#define driveenable()  asm volatile(  "cbi 0x0b,3 \n\t");
#define drivedisable() asm volatile(  "sbi 0x0b,3 \n\t");

/* LED13 on the Uno  Port B bit 5 => Arduino D13  */
#define led13on()   asm volatile("  sbi 0x05,5 \n");
#define led13off()  asm volatile("  cbi 0x05,5 \n");

/* debug LED D5 */
#define debugOn()   asm volatile("  sbi 0x0b,5 \n");
#define debugOff()  asm volatile("  cbi 0x0b,5 \n");

/* debug switch input */
#define RunStop   0x02  // Arduino D9  (PB1)

/* Read port B pin conditions */
unsigned char inportBpins(){
  unsigned char  captured;
  asm volatile(     "in %0,0x03\n\t"  : "=r"(captured) );
  return captured;
}
