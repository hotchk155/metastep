#include <SPI.h>
#include "PortIO.h"
#define P_SH_DAT  9
#define P_SH_CLK  4
#define P_KEY0    12
#define P_KEY1    10
#define P_KEY2    2 
#define P_STR_RED0   7
#define P_STR_RED1   3
#define P_STR_POTS0  6
#define P_STR_POTS1  8
#define P_STR_MODE0  5

#define P_POT_SEL0 17
#define P_POT_SEL1 16
#define P_POT_SEL2 15


byte ui_leds[16];
byte ui_disp[3];

volatile byte ui_scan_pos;
volatile byte ui_phase;

volatile byte ui_state;
volatile unsigned long ui_keys;
volatile unsigned long ui_prev_keys;
volatile unsigned long ui_key_press;
volatile unsigned long ui_key_index;
volatile byte ui_debounce;

// MENU BUTTONS
// A B      E
// C D      F
#define UI_KEY_A  (((unsigned long)1)<<22)
#define UI_KEY_B  (((unsigned long)1)<<23)
#define UI_KEY_C  (((unsigned long)1)<<20)
#define UI_KEY_D  (((unsigned long)1)<<21)
#define UI_KEY_E  (((unsigned long)1)<<16)
#define UI_KEY_F  (((unsigned long)1)<<17)

// INDICATOR LEDS
// A B      C
enum {
  UI_IND_A = 2,
  UI_IND_B = 1,
  UI_IND_C = 128
};

enum {
  UI_SCAN_BEGIN,
  UI_SCAN_STEP,
  UI_SCAN_WAIT,
  UI_DISP_BEGIN,
  UI_DISP_STEP,
  UI_DISP_WAIT,  
};

#define UI_MAX_DUTY 15
#define UI_DEBOUNCE 10

////////////////////////////////////////////////////////////////////////////////
//
// INTERRUPT SERVICE ROUTINE FOR UPDATING THE DISPLAY AND READING KEYBOARD
//
////////////////////////////////////////////////////////////////////////////////
ISR(TIMER2_OVF_vect) 
{  
  switch(ui_state)
  {
    case UI_SCAN_BEGIN:
      _DIGITAL_WRITE(P_STR_RED0, LOW);
      _DIGITAL_WRITE(P_STR_RED1, LOW);
      for(int i=0; i<8; ++i) {
        _DIGITAL_WRITE(P_SH_CLK, LOW);
        _DIGITAL_WRITE(P_SH_DAT, LOW);
        _DIGITAL_WRITE(P_SH_CLK, HIGH);
      }              
      _DIGITAL_WRITE(P_SH_CLK, LOW);
      _DIGITAL_WRITE(P_SH_DAT, HIGH);
      _DIGITAL_WRITE(P_SH_CLK, HIGH);
      ui_scan_pos = 0;
      ui_keys = 0;
      ui_state = UI_SCAN_STEP;
      break;
    
    ///////////////////////////////////
    case UI_SCAN_STEP:    
      ui_phase = 0;
      _DIGITAL_WRITE(P_SH_CLK, LOW);
      _DIGITAL_WRITE(P_SH_DAT, LOW);
      _DIGITAL_WRITE(P_SH_CLK, HIGH);
      if(ui_leds[7-ui_scan_pos])
        _DIGITAL_WRITE(P_STR_RED0, HIGH);
      if(ui_leds[15-ui_scan_pos])
        _DIGITAL_WRITE(P_STR_RED1, HIGH);
      ui_state = UI_SCAN_WAIT;
      break;
      
    ///////////////////////////////////
    case UI_SCAN_WAIT:
      ++ui_phase;        
      if(ui_phase >= UI_MAX_DUTY) {
        _DIGITAL_WRITE(P_STR_RED0, LOW);
        _DIGITAL_WRITE(P_STR_RED1, LOW);            
        if(_DIGITAL_READ(P_KEY0)) {
          ui_keys |= (((unsigned long)0x01) << ui_scan_pos);
        }
        if(_DIGITAL_READ(P_KEY1)) {
          ui_keys |= (((unsigned long)0x0100) << ui_scan_pos);
        }
        if(_DIGITAL_READ(P_KEY2)) {
          ui_keys |= (((unsigned long)0x010000) << ui_scan_pos);
        }
        if(ui_debounce) {
          --ui_debounce;            
        }       
        if(++ui_scan_pos >= 8) {          
          if(!ui_debounce) {            
            if(ui_keys != ui_prev_keys) {
              ui_key_press = ui_keys & ~ui_prev_keys;
              ui_prev_keys = ui_keys;
              ui_debounce = UI_DEBOUNCE;
            }
          }           
          ui_phase = 0;
          ui_scan_pos = 0;
//          ui_state = UI_SCAN_BEGIN;
          ui_state = UI_DISP_BEGIN;
        }
        else {
          ui_state = UI_SCAN_STEP;
        }
      }
      else
      {
        if(ui_phase >= ui_leds[7-ui_scan_pos])
          _DIGITAL_WRITE(P_STR_RED0, LOW);
        if(ui_phase >= ui_leds[15-ui_scan_pos])
          _DIGITAL_WRITE(P_STR_RED1, LOW);        
      }
      break;

    case UI_DISP_BEGIN:
      ui_scan_pos = 0;
      ui_state = UI_DISP_STEP;
      break;
      
    case UI_DISP_STEP:
      {
        _DIGITAL_WRITE(P_STR_POTS0, LOW);
        _DIGITAL_WRITE(P_STR_POTS1, LOW);
        _DIGITAL_WRITE(P_STR_MODE0, LOW);
        
        byte d = ui_disp[ui_scan_pos];
        _DIGITAL_WRITE(P_SH_CLK, LOW);
        _DIGITAL_WRITE(P_SH_DAT, LOW);
        _DIGITAL_WRITE(P_SH_CLK, HIGH);
        for(int i=0; i<8; ++i) {
          _DIGITAL_WRITE(P_SH_CLK, LOW);
          _DIGITAL_WRITE(P_SH_DAT, (d&1));
          _DIGITAL_WRITE(P_SH_CLK, HIGH);
          d>>=1;
        }        
        _DIGITAL_WRITE(P_SH_CLK, LOW);
        _DIGITAL_WRITE(P_SH_DAT, LOW);
        _DIGITAL_WRITE(P_SH_CLK, HIGH);
        _DIGITAL_WRITE(P_STR_POTS0, (ui_scan_pos == 0));
        _DIGITAL_WRITE(P_STR_POTS1, (ui_scan_pos == 1));
        _DIGITAL_WRITE(P_STR_MODE0, (ui_scan_pos == 2));        
        ui_phase = 0;
        ui_state = UI_DISP_WAIT;
      }
      break;

    ///////////////////////////////////
    case UI_DISP_WAIT:
      if(++ui_phase >= UI_MAX_DUTY) {
        _DIGITAL_WRITE(P_STR_POTS0, LOW);
        _DIGITAL_WRITE(P_STR_POTS1, LOW);
        _DIGITAL_WRITE(P_STR_MODE0, LOW);
        if(++ui_scan_pos >= 3) {          
          ui_state = UI_SCAN_BEGIN;
        }
        else {
          ui_state = UI_DISP_STEP;
        }
      }
      break;      
  }        
  TCNT2 = 200;
}    

void ui_init()
{
  memset(ui_leds, 0, sizeof(ui_leds));
  memset(ui_disp, 0, sizeof(ui_disp));

  ui_state = UI_SCAN_BEGIN;
  ui_prev_keys = 0;
  ui_key_press = 0;
  ui_debounce = 0;
  
  // start the interrupt to service the UI   
  TCCR2A = 0;
  TCCR2B = 1<<CS21 | 1<<CS20;
  TIMSK2 = 1<<TOIE2;
  TCNT2 = 0; 
  
}

////////////////////////////////////////////////////////////////////////////////
void ui_set_pot_led(byte n, boolean on) {
  if(n<5) {
    if(on) {
      ui_disp[0] |= (4 << n);
    } else {
      ui_disp[0] &= ~(4 << n);
    }    
  }
  else  if(n<10) {
    if(on) {
      ui_disp[1] |= (4 << (n-5));
    } else {
      ui_disp[1] &= ~(4 << (n-5));
    }    
  }
}

////////////////////////////////////////////////////////////////////////////////
void ui_clear_pot_leds() {
  ui_disp[0] = 0;
  ui_disp[1] = 0;
}

////////////////////////////////////////////////////////////////////////////////
void ui_set_indicators(byte n) {
  ui_disp[2] |= n;
}

////////////////////////////////////////////////////////////////////////////////
void ui_clear_indicators(byte n) {
  ui_disp[2] &= ~n;
}

void ui_clear_step_leds()
{
  memset(ui_leds, 0, sizeof(ui_leds));
  
}

void ui_set_step_led(int n, byte i)
{
  if(n<8) {
    ui_leds[7-n] = i;
  }
  else if(n<16) {
    ui_leds[23-n] = i;
  }
  
/*  00111111
  89012345
  
  11111100
  54321098
  
  15-n+8)*/
}

unsigned int pots[10];
byte potIndex = 0;

void potsInit() 
{
  memset(&pots, 0, sizeof(pots));  
  pinMode(P_POT_SEL0, OUTPUT);
  pinMode(P_POT_SEL1, OUTPUT);
  pinMode(P_POT_SEL2, OUTPUT);  
}
void potsRun() 
{  
  digitalWrite(P_POT_SEL0, !!(potIndex&0x01));
  digitalWrite(P_POT_SEL1, !!(potIndex&0x02));
  digitalWrite(P_POT_SEL2, !!(potIndex&0x04));  
 
  byte m=0,n = 0;
  switch(potIndex){
    case 0: m=2; n=7; break;
    case 1: m=3; n=6; break;
    case 2: m=4; n=5; break;
    case 3: m=1; n=8; break;
    case 4: m=0; n=9; break;
  }
  
  pots[m] = 0.9 * pots[m] + 0.1 * analogRead(4);
  pots[n] = 0.9 * pots[n] + 0.1 * analogRead(5);
  if(++potIndex >= 5) {
    potIndex = 0;
  }
}

#define P_DAC_CS 10 //?
#define P_GATE 14


unsigned long cvGateEnd;
void cvInit()
{
  pinMode(P_GATE, OUTPUT);
  pinMode(P_DAC_CS, OUTPUT);
  digitalWrite(P_DAC_CS, HIGH);
  digitalWrite(P_GATE, LOW);
  SPI.begin();    
  cvGateEnd = 0;
}

void cvOpenGate(unsigned long milliseconds, int duration) {
  cvGateEnd = milliseconds + duration;
  digitalWrite(P_GATE, HIGH);
}

void cvWriteDAC(byte which, int d)
{
  digitalWrite(P_DAC_CS, LOW);
  unsigned int w = (d & 0b0000111111111111);
  w |= 0b0111000000000000;
  if(which) w |= 0b1000000000000000;
  SPI.transfer((w >> 8) & 0xff );
  SPI.transfer(w & 0xff);
  digitalWrite(P_DAC_CS, HIGH);
}

void cvRun(unsigned long milliseconds) {
  if(cvGateEnd && milliseconds >= cvGateEnd) {
    cvGateEnd = 0;
    digitalWrite(P_GATE, LOW);
  }
}
  

int  seqStep;
double seqNextStep;
double seqPeriod;
byte seqTrigs[16];
void seqInit()
{
  memset(seqTrigs,0,sizeof(seqTrigs));
  for(int i=0; i<10; ++i) {
    seqTrigs[i] = 1;
  }

  memcpy(ui_leds, seqTrigs, sizeof ui_leds);

  
  seqStep = 0;
  seqNextStep = 0;
  seqPeriod = 220 ;  
}
void seqDisplay() 
{
  memset(ui_leds, 0, sizeof(ui_leds));  
  for(int i=0; i<10; ++i) {
    int j=(i>4)? i+3: i;
    if(seqTrigs[i]) {
       ui_leds[j] = 1;
    }
    if(i==seqStep) {
       ui_leds[j] = 15;
    }
  }
}
void seqRun(unsigned long milliseconds) {
  if(milliseconds > seqNextStep) {
    if(++seqStep >= 10)
      seqStep = 0;
    ui_clear_pot_leds();
    ui_set_pot_led(seqStep, true);
    seqDisplay();
    
    cvOpenGate(milliseconds, 100);
    seqNextStep = milliseconds + seqPeriod;
    
  }
    unsigned int v = pots[seqStep];
    v *=2;
    v = 1500+v;
    cvWriteDAC(0, 4095-v);

    if(0)//ui_key_press)
    {
      for(int i=0; i<16; ++i) {
        if(ui_key_press & ((unsigned long)1<<i)) {
          seqTrigs[i] = !seqTrigs[i];
        }
      }
      seqDisplay();
      ui_key_press = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
void setup()
{

  pinMode(P_SH_DAT, OUTPUT);
  pinMode(P_SH_CLK, OUTPUT);
  pinMode(P_STR_RED0, OUTPUT);
  pinMode(P_STR_RED1, OUTPUT);
  pinMode(P_STR_POTS0, OUTPUT);
  pinMode(P_STR_POTS1, OUTPUT);
  pinMode(P_STR_MODE0, OUTPUT);
  pinMode(P_KEY0, INPUT);
  pinMode(P_KEY1, INPUT);
  pinMode(P_KEY2, INPUT);
  
  digitalWrite(P_SH_DAT, LOW);
  digitalWrite(P_SH_CLK, LOW);
  digitalWrite(P_STR_RED0, LOW);
  digitalWrite(P_STR_RED1, LOW);
  digitalWrite(P_STR_POTS0, LOW);
  digitalWrite(P_STR_POTS1, LOW);
  digitalWrite(P_STR_MODE0, LOW);

 
  potsInit();
  cvInit();
  ui_init();
  seqInit();  
  seqDisplay();

}

void loop() 
{
  
   unsigned long milliseconds = millis();
   potsRun();
   seqRun(milliseconds);
   cvRun(milliseconds);
}






