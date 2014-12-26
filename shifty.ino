
#include "shifty.h"

unsigned char inportBpins(){
  unsigned char  captured;
  asm volatile(     "in %0,0x03\n\t"  : "=r"(captured) );
  return captured;
}

/*
   A dot shifts across the LED array by jumping from one position to the next.
   After jumping, it glows only for its "on" duration, then blanks.
   After traversing the array, it stalls for a while before beginning again.
   The period between jumps slowly increases until it hits a final value,
   which resets it to an initial value.
   A dot is represented by a bit pattern, 1 means on.
 */
#define Nd 2  //  Number of dots
   
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
static DOTZ d;


void setup(){
  unsigned char charout =PBDIRR; //DDRB=4
  unsigned char charin  =0;
  //                       /<-  ATmega328P pin numbers 
  pinMode( 13, OUTPUT); // 19 (PB5) D13 onboard LED HIGH=on
  pinMode( 10, OUTPUT); // 16 (PB2) unused  Arduino pin
  pinMode(  9,  INPUT); // 15 (PB1) RunStop switch
  pinMode(  8, OUTPUT); // 14 (PB0) (SR input clock)
  pinMode(  7, OUTPUT); // 13 D7 (serial in to SR)
  pinMode(  6, OUTPUT); // 12 D6 (serial in to SR2)
  pinMode(  5, OUTPUT); // 11 D5 (debug LED)
  pinMode(  3, OUTPUT); //  5 D3 (SR oe_)
  pinMode(  2, OUTPUT); //  4 D2 (SR output clock)
  Serial.begin(9600);
 // Serial.print(sizeof(int),DEC);
  debugOff();
  led13off();

  //  starting patterns
  d.initialSprite[0]=  0x0003;
  d.sprite[0] = d.initialSprite[0];
  d.initialSprite[1]=  0x0030;
  d.sprite[1] = d.initialSprite[1];
  d.previousSprite[0]=  0x0000;
  d.previousSprite[1]=  0x0000;
  d.mask[0] = 0xffff  ;
  d.mask[1] = 0xffff  ;
  d.stallMask[0] = 0xffff;
  d.stallMask[1] = 0xffff;

  //durations, in times around loop() 
  d.initialTickPeriod[0] = 1200  ;
  d.finalTickPeriod[0] = 2000 ;
  d.blankAfter[0] = 1000   ;
  d.stallDuration[0] = 20000;
  d.jumpTimer[0] = 1;
  d.onTimer[0] = 1;
  d.stallTimer[0] = 0;

  d.initialTickPeriod[1] =  2000  ;
  d.finalTickPeriod[1] = 5000 ;
  d.blankAfter[1] = 1000   ;
  d.stallDuration[1] = 11400;
  d.jumpTimer[1] = 1;
  d.onTimer[1] = 1;
  d.stallTimer[1] = 0;

  d.currentTickPeriod[0] = d.initialTickPeriod[0];
  d.currentTickPeriod[1] = d.initialTickPeriod[1];
}


/* durations, in times around loop() */
int pwmCyclePeriod = 74 ;
int pwmOnDuration =   10;
int analogReadPeriod = 100;

/* free running timers */
int pwmCycleTimer =1;
int analogReadCounter = 1;

/* one shot timers */
int pwmOnTimer =2 ;
  
/* bit patterns */
unsigned short displayImage = 0x1111;
unsigned short previousDisplayImage = 0x7777;
/* debug statistics */
int recordLow =500;
int recordHigh=501;

/*
    Display a pattern on the T5s
 */
void display16( unsigned short toshiftreg ){
  unsigned char ix;
  
  for( ix=8 ; ix > 0; ix-- ){
    if(toshiftreg & 0x0001){ deeHigh() }else{ deeLow() };
    if(toshiftreg & 0x0100){ dee8High() }else{ dee8Low() };
    srclkpulse();
    toshiftreg = toshiftreg >> 1; 
  }
  clockitout();
}

/*
     Circular shift.  There's no AVR instruction.
 */
unsigned short rotateright(unsigned short rotateme){
  unsigned short carry;
  if (rotateme & 0x0001) {
    carry = 0x8000;
  }else{
    carry = 0;
  }
  return( (rotateme >> 1) | carry );
}
unsigned short rotateleft(unsigned short rotateme){
  unsigned short carry;
  if (rotateme & 0x8000) {
    carry = 0x0001;
  }else{
    carry = 0;
  }
  return( (rotateme << 1) | carry );
}

// even numbered dots rotate left.
void dotJumpTimerService(unsigned char which){
  if( d.stallTimer[which] ==0 ){
       d.previousSprite[which]  = d.sprite[which];
       if( which  &  0x01 ){
         d.sprite[which]  = rotateleft(d.sprite[which]);
       }else{
         d.sprite[which]  = rotateright(d.sprite[which]);
       }
  }
  d.jumpTimer[which] = d.currentTickPeriod[which];
  d.onTimer[which] = d.blankAfter[which];
  d.mask[which] = 0xffff;
  if( (d.sprite[which] == d.initialSprite[which])  && (d.stallTimer[which] == 0)  ){
    debugOff();
    d.stallTimer[which] = d.stallDuration[which];
  }
}

void dotStallTimerService(unsigned char which){
  d.stallMask[which] = 0x0000;
  d.stallTimer[which]--;
  if ( d.stallTimer[which] == 0  ){
    d.currentTickPeriod[which] = d.currentTickPeriod[which] + (d.currentTickPeriod[which]/2);
    if( d.currentTickPeriod[which] > d.finalTickPeriod[which] ){
      d.currentTickPeriod[which] = d.initialTickPeriod[which]   /* d.initialTickPeriod[which] */  ;
    }
  }
}
 

void pwmCycleTimerService(){
  pwmOnTimer  = pwmOnDuration;
  pwmCycleTimer = pwmCyclePeriod;
  driveenable(); 
}

void analogReadCounterService(){
  short potvalue;

  potvalue = analogRead( potwiper );
  pwmOnDuration = (potvalue / 10) + 1 ; 
  analogReadCounter = analogReadPeriod;
}

void loop(){
  unsigned char ix;

  for( ix=0; ix < Nd; ix++){
    if( d.stallTimer[ix] != 0  ){
      dotStallTimerService(ix);
    }else{
      d.stallMask[ix] = 0xffff;
    }
    if( --d.jumpTimer[ix] ==0 ){
      dotJumpTimerService(ix);
    }
    if( d.onTimer[ix] ){
      d.onTimer[ix]-- ;
      if( d.onTimer[ix] ==0 ){
        d.mask[ix] = 0x0000;
      }
    }
  }

  /*  highly sophisticated compositing algorithm  */
  displayImage = d.sprite[0] & d.mask[0] & d.stallMask[0]  |  d.sprite[1] & d.mask[1] & d.stallMask[1] ;
                 d.previousSprite[0] & d.mask[0] & d.stallMask[0]  |  d.previousSprite[1] & d.mask[1] & d.stallMask[1] ;

  if( --pwmCycleTimer == 0){
    pwmCycleTimerService();
  }
  if( pwmOnTimer  ){
    pwmOnTimer--;
    if( pwmOnTimer == 0 ){
      drivedisable();
    }
  }

  if( --analogReadCounter == 0 ){
     analogReadCounterService();
  }

  // wait here for run/stop switch
  while( (inportBpins() & 0x02) == 0x02)
  {
    ;
  }
led13on() ; // delay(100);
 //   driveenable(); 
led13off() ; //  delay(100);
//    drivedisable();

  if( displayImage != previousDisplayImage ){
    display16(displayImage);
    previousDisplayImage  = displayImage;
  }
}

