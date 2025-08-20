#include <SPI.h>
#include <stdio.h>
// cosntants
// ----------------------------------------------------------------------------------------------------------------------------------------
// if using a differnet micocontroller just change pin numbers code is standard c++ except SPI so thats the only things you need to change
// you might be wondering why we went with bindary instead of one hot encoding, we didnt have enough pins and neither will you unless you use 
// a differnet microcontroller like some esp32 or arduino mega

//Latch for LED Driver
const int latchPin = 10;

//buttons
const int key1 = 2; //left most key
const int key2 = 3;
const int key3 = 4;
const int key4 = 5; //right most key

//music binary
const int m1 = 6; 
const int m2 = 7;
const int m3 = 8;
const int m4 = 9;
//little endian bit 0 is index 0 

// this is also in binary to choose the row for the lights
const int r1 = A0;
const int r2 = A1; //00 top of breadboard 01 second top

// grid info
const int length = 4;

// to change note order just change the notes array
const uint8_t notes[] = {3, 2, 1, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 4, 4, 4, 3, 2, 1, 2, 3, 3, 3, 3, 2, 2, 3, 2, 1, 1, 1, 1}; //adjust notes so it doesnt need durations
const int song_length = 32;

uint8_t noteQueue[length] = {0, 0, 0, 0}; // 4 rows of notes (left is top, right is bottom) bitmask uint8_t
int noteIndex = 0;

const uint8_t winP[length] = {10, 5, 10, 5}; //checkerboard pattern for win

// song speed info
int interval = 1000;
int level = 1;
// ----------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  pinMode(latchPin, OUTPUT);

  pinMode(key1, INPUT);
  pinMode(key2, INPUT);
  pinMode(key3, INPUT);
  pinMode(key4, INPUT);

  pinMode(m1, OUTPUT);
  pinMode(m2, OUTPUT);
  pinMode(m3, OUTPUT);
  pinMode(m4, OUTPUT);

  digitalWrite(m1, HIGH); //LOW ON : HIGH OFF
  digitalWrite(m2, HIGH);
  digitalWrite(m3, HIGH);
  digitalWrite(m4, HIGH); 

  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(latchPin, LOW); 

  SPI.transfer(0b11111111); //second half is the only important data 0000xxxx where x is the data we need to send

  digitalWrite(latchPin, HIGH); 
}

void advanceQueue() {
  uint8_t expected = noteQueue[length - 1];
  bool matched = false;

  const int keys[4] = {key4, key3, key2, key1};  // corresponds to bits 0–3
  uint8_t inputMask = 0;

  for (int i = 0; i < 4; i++) { //bitmask type shit
    if (isKeyDown(keys[i])) {
      inputMask |= (1 << i);
    }
  }

  int count = 0;
  for (int i = 0; i < 4; i++) {
    if (inputMask & (1 << i)) count++;
  }
  if (count == 1 && inputMask == expected) {
    matched = true;
  }

  if (matched || expected == 0) {

    // Shift all rows down
    for (int i = length - 1; i > 0; i--) {
      noteQueue[i] = noteQueue[i - 1];
    }

    playNote(expected);

    // Add next note to the top if available
    if (noteIndex < song_length) {
      noteQueue[0] = key_to_bit(notes[noteIndex]);
      noteIndex++;
    } else if (isEmpty(noteQueue, length)){
      win(level); //this only happens when you reach the end of the song
    } else {
      // for end of song
      noteQueue[0] = 0;
    }
  } else {
    lose();
  }
}

int currentRow = 0;
unsigned long lastRowUpdate = 0;
const int rowInterval = 1;  //delay between refreshing a row and the next row.

void playNote(uint8_t note){
  digitalWrite(m1, HIGH); //LOW ON : HIGH OFF
  digitalWrite(m2, HIGH);
  digitalWrite(m3, HIGH);
  digitalWrite(m4, HIGH); 

  if (note != -1) {
    if (note == 8) {
      digitalWrite(m1, LOW);
    } else if (note == 4) {
      digitalWrite(m2, LOW);
    } else if (note == 2) {
      digitalWrite(m3, LOW);
    } else if (note == 1){
      digitalWrite(m4, LOW);
    }
  }

  
    
  
}

void drawOneRow() {
  light(currentRow, noteQueue[currentRow]);
  currentRow = (currentRow + 1) % length;
}

void win(int level){

  // cut off sound
  playNote(-1);


  delay(1500); 


  while (1) {

    winPattern();

    if (isKeyDown(key1)) {
      level = level + 1;
      interval /= (level);
      break;
    } 
  }


  reset();
}


void winPattern(){
  for (int row = 0; row < length; row++) {
    light(row, winP[row]);
    delay(2);
  }
}

void lose() {

  // cut off sound
  playNote(-1);

  delay(1500); 


  while (1) {

    losePattern();

     if (isKeyDown(key1) && isKeyDown(key4)) {
      level = level + 1;
      interval /= (level);
      break;
    } else if (isKeyDown(key1)) {
      level = 1;
      interval = 1000;
      break;
    }
  }


  reset();
}

boolean isEmpty(uint8_t* array, int size) {
  bool empty = true;
  for (int i = 0; i < size; i++) {
    if (*(array+i) != 0) {
      empty = false;
    }
  }
  return empty;
}

void losePattern() {
  // Step 1: Show full screen (all rows ON) for 300 ms
  unsigned long start = millis();
  while (millis() - start < 300) {
    for (int row = 0; row < 4; row++) {
      light(row, 0b1111);  // All LEDs in this row ON
      delay(2);            // Tiny delay to simulate multiplexing
    }
  }

  // Step 2: Crumble down one row at a time (clear row after row)
  for (int r = 0; r < 4; r++) {
    unsigned long crumbleStart = millis();
    while (millis() - crumbleStart < 100) {
      for (int row = 0; row < 4; row++) {
        if (row >= r) {
          light(row, 0b1111);  // Still ON
        } else {
          light(row, 0b0000);  // Already crumbled
        }
        delay(2);  // Keep multiplexing visible
      }
    }
  }

  // Step 3: Fully off
  for (int i = 0; i < 100; i++) {
    for (int row = 0; row < 4; row++) {
      light(row, 0b0000);
      delay(2);
    }
  }
}


void reset() {
  
  noteIndex = 0;
  for (int i = 0; i < length; i++) {
    noteQueue[i] = 0;
  }
  
}

uint8_t key_to_bit(int key) { // take binary and shift simple really
  return 8 >> (key - 1);
}

void light(int row, int bit) {

  // bit is defined from 0 - 15

  if (row == 0) {
    digitalWrite(r1, LOW);
    digitalWrite(r2, LOW);
  } else if (row == 1) {
    digitalWrite(r1, HIGH);
    digitalWrite(r2, LOW);
  } else if (row == 2) {
    digitalWrite(r1, LOW);
    digitalWrite(r2, HIGH);
  } else if (row == 3) {
    digitalWrite(r1, HIGH);
    digitalWrite(r2, HIGH);
  }

  digitalWrite(latchPin, LOW); 

  SPI.transfer(bit); //second half is the only important data 0000xxxx where x is data is only needed

  digitalWrite(latchPin, HIGH);
}

int isKeyDown(int key) {
  return digitalRead(key);
}


unsigned long lastUpdate = 0;



//main loop -------------------------------------------------------------------------------------------------------------------------------

void loop() {
  unsigned long now = millis();

  if (now - lastRowUpdate >= rowInterval) {
    lastRowUpdate = now;
    drawOneRow();  // refresh one row per frame
  }

  if (now - lastUpdate >= interval) {
    lastUpdate = now;
    advanceQueue();  // scroll new notes downward
  }


  // playNote(4);
  // delay(1000);

  
}
