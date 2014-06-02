
#include <MIDI.h>


//define where your pins are
int latchPin = 8;
int dataPin = 9;
int clockPin = 7;

int csensorPin = 12;

byte buttonStates = 255;  //01001000

void setup() {
  //define pin modes
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT); 
  pinMode(dataPin, INPUT);

  MIDI.begin(4); //TODO: Check if we even need that call when we're only sending MIDI and not receiving.

}

const float csensor_threshold = 0.25f;

void loop() {
  if (readCSensor() > csensor_threshold)
  {
    buttonStates = getButtonStates();
    
    byte notes[6];
    
    getNotes(buttonStates, notes);
    
    play(notes, 6);
  }  
}


byte getButtonStates()
{

  //Pulse the latch pin:
  //set it to 1 to collect parallel data
  digitalWrite(latchPin,1);
  //set it to 1 to collect parallel data, wait
  delayMicroseconds(20);
  //set it to 0 to transmit data serially  
  digitalWrite(latchPin,0);

  int i;
  int temp = 0;
  int pinState;
  byte dataIn = 0;

  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);

//at the begining of each loop when we set the clock low, it will
//be doing the necessary low to high drop to cause the shift
//register's DataPin to change state based on the value
//of the next bit in its serial information flow.
//The register transmits the information about the pins from pin 7 to pin 0
//so that is why our function counts down
  for (i=7; i>=0; i--)
  {
    digitalWrite(clockPin, 0);
    delayMicroseconds(0.2);
    temp = digitalRead(dataPin);
    if (temp) {
      pinState = 1;
      //set the bit to 0 no matter what
      dataIn = dataIn | (1 << i);
    }
    digitalWrite(clockPin, 1);
  }
  
  return dataIn;
}

const byte base_note = 44;
const int note_delay = 1500;
const int string_delay = 10;
const int note_velocity = 64;

void play(byte* notes, int length)
{
    for (int i = 0; i < length; i++)
    {
        byte note = notes[i];
        if (note > 0)
        {
            MIDI.sendNoteOn(base_note + note,note_velocity,1);
            delay(string_delay);
        }
    }
    
    delay(note_delay);
    
    for (int i = 0; i < length; i++)
    {
        byte note = notes[i];
        if (note > 0)
        {
            MIDI.sendNoteOff(base_note + note,0,1);
            delay(string_delay);
        }
    }
}

/*
void getNotes(byte buttonStates, byte* notes)
{  
  
  notes[0] = 1;
  notes[1] = 5;
  notes[2] = 8;
  notes[3] = 13;
  notes[4] = 17;
  notes[5] = 20;
}*/

void getNotes(byte data, byte notes[])
{
  // Button1: A
  // Button2: C
  // Button3: E
  // Button4: G
  // Button5: Key+1
  // Button6: b/#
  // Button7: Minor
  // Button7: Seventh
  
  char key;
  byte diff;
  byte sharp = 0;
  byte minor = 0;
  byte sevth = 0;
  
  if ((B10000000 & data) == B10000000)
      key = 'A';
  if ((B01000000 & data) == B01000000)
      key = 'C';
  if ((B00100000 & data) == B00100000)
      key = 'E';
  if ((B00010000 & data) == B00010000)
      key = 'G';

  if ((B00001000 & data) == B00001000)
    key += 1;
  if (key == 'H')
    key = 'A';

  sharp = (B00000100 & data) == B00000100;  
  minor = (B00000010 & data) == B00000010;
  sevth = (B00000001 & data) == B00000001;

  diff = (key - 'A') * 2;
  if (diff > 11)
    diff -= 2;
  else if (diff > 3)
    diff--;
    
  notes[0] = 1 + diff;
  notes[1] = 5 + diff;
  notes[2] = 8 + diff;
  notes[3] = 13 + diff;
  notes[4] = 17 + diff;
  notes[5] = 20 + diff;
  if (minor)
  {
    notes[1] -= 1;
    notes[4] -= 1;
  }
  if (sevth)
    notes[3] -= 2;
  if (sharp)
  {
    int tmp = 1;
    if (key == 'B' || key == 'E')
      tmp = -1;
    notes[0] += tmp;
    notes[1] += tmp;
    notes[2] += tmp;
    notes[3] += tmp;
    notes[4] += tmp;
    notes[5] += tmp;
  }
}

float readCSensor()
{
  return (float)readCapacitivePin(csensorPin) / 17.0;
}

// readCapacitivePin
//  Input: Arduino pin number
//  Output: A number, from 0 to 17 expressing
//  how much capacitance is on the pin
//  When you touch the pin, or whatever you have
//  attached to it, the number will get higher
uint8_t readCapacitivePin(int pinToMeasure) {
  // Variables used to translate from Arduino to AVR pin naming
  volatile uint8_t* port;
  volatile uint8_t* ddr;
  volatile uint8_t* pin;
  // Here we translate the input pin number from
  //  Arduino pin number to the AVR PORT, PIN, DDR,
  //  and which bit of those registers we care about.
  byte bitmask;
  port = portOutputRegister(digitalPinToPort(pinToMeasure));
  ddr = portModeRegister(digitalPinToPort(pinToMeasure));
  bitmask = digitalPinToBitMask(pinToMeasure);
  pin = portInputRegister(digitalPinToPort(pinToMeasure));
  // Discharge the pin first by setting it low and output
  *port &= ~(bitmask);
  *ddr  |= bitmask;
  delay(1);
  // Prevent the timer IRQ from disturbing our measurement
  noInterrupts();
  // Make the pin an input with the internal pull-up on
  *ddr &= ~(bitmask);
  *port |= bitmask;

  // Now see how long the pin to get pulled up. This manual unrolling of the loop
  // decreases the number of hardware cycles between each read of the pin,
  // thus increasing sensitivity.
  uint8_t cycles = 17;
       if (*pin & bitmask) { cycles =  0;}
  else if (*pin & bitmask) { cycles =  1;}
  else if (*pin & bitmask) { cycles =  2;}
  else if (*pin & bitmask) { cycles =  3;}
  else if (*pin & bitmask) { cycles =  4;}
  else if (*pin & bitmask) { cycles =  5;}
  else if (*pin & bitmask) { cycles =  6;}
  else if (*pin & bitmask) { cycles =  7;}
  else if (*pin & bitmask) { cycles =  8;}
  else if (*pin & bitmask) { cycles =  9;}
  else if (*pin & bitmask) { cycles = 10;}
  else if (*pin & bitmask) { cycles = 11;}
  else if (*pin & bitmask) { cycles = 12;}
  else if (*pin & bitmask) { cycles = 13;}
  else if (*pin & bitmask) { cycles = 14;}
  else if (*pin & bitmask) { cycles = 15;}
  else if (*pin & bitmask) { cycles = 16;}

  // End of timing-critical section
  interrupts();

  // Discharge the pin again by setting it low and output
  //  It's important to leave the pins low if you want to 
  //  be able to touch more than 1 sensor at a time - if
  //  the sensor is left pulled high, when you touch
  //  two sensors, your body will transfer the charge between
  //  sensors.
  *port &= ~(bitmask);
  *ddr  |= bitmask;

  return cycles;
}
