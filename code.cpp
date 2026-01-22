#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {10, 11, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int ldrPin = A0;
const int redLedPin = 5;
const int greenLedPin = 4;
const int blueLedPin = 3;
const int buzzerPin = 2;
const int potPin = A2;
const int soundSensorPin = A1;

int t = 200; // Default speed (Medium)
int mode = -1;
String inputText = "";
int lastSpeedMode = 1; // 0=Slow, 1=Medium, 2=Fast
bool speedAdjustMode = false;

// Morse Code Tables
const char* morseTable[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

const char* morseCodeMap[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

const char lettersAndNumbers[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char* t9Keys[] = {
  "0", "ABC1", "DEF2", "GHI3", "JKL4", "MNO5", "PQR6", "STU7", "VWX8", "YZ9"
};

char lastKey = '\0';
int keyPressCount = 0;
unsigned long lastKeyTime = 0;

// Function prototypes
void displayMainMenu();
void updateSpeed();
void adjustSpeed();
void encodeMorse(String message);
char decodeChar(String morse);
void decodeMorseLightSensor();
void handleT9Input(char key);
void decodeMorseWithSound();

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(potPin, INPUT);
  pinMode(ldrPin, INPUT);
  pinMode(soundSensorPin, INPUT);

  // Test LEDs and buzzer
  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(blueLedPin, HIGH);
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(buzzerPin, LOW);

  displayMainMenu();
}

void displayMainMenu() {
  Serial.println("\n--- Morse Code System ---");
  Serial.println("A: Encode Mode");
  Serial.println("B: Sound Decode Mode");
  Serial.println("C: Set Speed");
  Serial.println("D: Light Decode Mode");
  Serial.println("-------------------------");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("A:Encode B:Sound");
  lcd.setCursor(0, 1);
  lcd.print("C:Speed D:Light");

  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  mode = -1;
  speedAdjustMode = false;
}

void updateSpeed() {
  int potValue = analogRead(potPin);
  int newSpeedMode;

  if (potValue < 341) {
    t = 500; // Slow
    newSpeedMode = 0;
  } else if (potValue < 1000) {
    t = 250; // Medium
    newSpeedMode = 1;
  } else {
    t = 120; // Fast
    newSpeedMode = 2;
  }

  if (speedAdjustMode && newSpeedMode != lastSpeedMode) {
    lastSpeedMode = newSpeedMode;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Adjust Speed:");
    lcd.setCursor(0, 1);
    switch(newSpeedMode) {
      case 0:
        lcd.print("SLOW (500ms)");
        Serial.println("Speed changed to SLOW (500ms)");
        break;
      case 1:
        lcd.print("MEDIUM (250ms)");
        Serial.println("Speed changed to MEDIUM (250ms)");
        break;
      case 2:
        lcd.print("FAST (120ms)");
        Serial.println("Speed changed to FAST (120ms)");
        break;
    }

    digitalWrite(greenLedPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
}

void adjustSpeed() {
  Serial.println("\n--- Adjust Speed ---");
  Serial.println("Rotate potentiometer to change speed");
  Serial.println("Press # to confirm and exit");

  lcd.clear();
  lcd.print("Adjust Speed:");
  lcd.setCursor(0, 1);
  lcd.print("Rotate Pot");

  speedAdjustMode = true;
  unsigned long startTime = millis();
  while (speedAdjustMode) {
    updateSpeed();

    char key = keypad.getKey();
    if (key == '#') {
      speedAdjustMode = false;
      break;
    }
    delay(100);
  }

  displayMainMenu();
}

void encodeMorse(String message) {
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  Serial.println("\n--- Encoding ---");
  Serial.print("Input: ");
  Serial.println(message);

  lcd.clear();
  lcd.print("Encoding...");

  Serial.print("Morse: ");
  for (int i = 0; i < message.length(); i++) {
    char letter = toupper(message[i]);
    int index = -1;
    if (letter >= 'A' && letter <= 'Z') {
      index = letter - 'A';
    } else if (letter >= '0' && letter <= '9') {
      index = 26 + (letter - '0');
    }

    if (letter == ' ') {
      Serial.print(" / ");
      delay(5 * t);
    } else if (index != -1) {
      String morseCode = morseTable[index];
      Serial.print(morseCode);
      Serial.print(" ");
      for (int j = 0; j < morseCode.length(); j++) {
        digitalWrite(blueLedPin, HIGH);
        digitalWrite(buzzerPin, HIGH);
        delay(morseCode[j] == '.' ? t : 3 * t);
        digitalWrite(blueLedPin, LOW);
        digitalWrite(buzzerPin, LOW);
        delay(t);
      }
      delay(2 * t);
    } else {
      Serial.print("?");
    }
  }
  noTone(buzzerPin);
  digitalWrite(greenLedPin, LOW);
  Serial.println("\n--- Encoding Done ---");
  lcd.clear();
  lcd.print("Encoding Done!");
  delay(2000);
  displayMainMenu();
}

char decodeChar(String morse) {
  for (int i = 0; i < 36; i++) {
    if (strcmp(morse.c_str(), morseCodeMap[i]) == 0) {
      return lettersAndNumbers[i];
    }
  }
  return '?';
}

void decodeMorseLightSensor() {
  Serial.println("\n--- Light Decoding ---");
  Serial.println("Shine light. Press # at the end of the message.");
  lcd.clear();
  lcd.print("Decoding...");

  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);

  String currentMorse = "";
  String decodedMessage = "";
  unsigned long lastLightChangeTime = millis();
  unsigned long lastSignalEndTime = millis();
  bool signalState = false;

  while (mode == 1) {
    char key = keypad.getKey();
    if (key == '#') {
      if (currentMorse.length() > 0) {
        char decodedChar = decodeChar(currentMorse);
        decodedMessage += decodedChar;
        Serial.print(decodedChar);
      }
      Serial.println("\n--- Decoding Stopped ---");
      Serial.print("Final Message: ");
      Serial.println(decodedMessage);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Decoded:");
      lcd.setCursor(0, 1);
      lcd.print(decodedMessage);

      for (int i = 0; i < 3; i++) {
        digitalWrite(blueLedPin, HIGH);
        tone(buzzerPin, 1000, 100);
        delay(200);
        digitalWrite(blueLedPin, LOW);
        delay(200);
      }

      delay(3000);
      displayMainMenu();
      break;
    }

    int ldrValue = analogRead(ldrPin);
    unsigned long currentTime = millis();
    unsigned long duration = currentTime - lastLightChangeTime;

    if (ldrValue < 100 && !signalState) {
      signalState = true;
      unsigned long spaceDuration = currentTime - lastSignalEndTime;

      if (spaceDuration > t * 3 && currentMorse.length() > 0) {
        char decodedChar = decodeChar(currentMorse);
        decodedMessage += decodedChar;
        Serial.print(decodedChar);
        currentMorse = "";

        if (spaceDuration > t * 7) {
          decodedMessage += ' ';
          Serial.print(" ");
        }
      }
      lastLightChangeTime = currentTime;
    } else if (ldrValue >= 100 && signalState) {
      signalState = false;
      lastSignalEndTime = currentTime;

      if (duration < t * 2) {
        currentMorse += '.';
        Serial.print(".");
      } else {
        currentMorse += '-';
        Serial.print("-");
      }
      lastLightChangeTime = currentTime;
    }

    delay(10);
  }
}

void handleT9Input(char key) {
  if (key < '0' || key > '9') return;
  int keyIndex = key - '0';
  int charCount = strlen(t9Keys[keyIndex]);
  if (charCount == 0) return;

  unsigned long currentTime = millis();
  if (key != lastKey || currentTime - lastKeyTime > 1000) {
    inputText += t9Keys[keyIndex][0];
    keyPressCount = 0;
  } else {
    keyPressCount = (keyPressCount + 1) % charCount;
    inputText[inputText.length() - 1] = t9Keys[keyIndex][keyPressCount];
  }

  lastKey = key;
  lastKeyTime = currentTime;

  Serial.print("Typing: ");
  Serial.println(inputText);

  lcd.clear();
  lcd.print("Typing:");
  lcd.setCursor(0, 1);
  lcd.print(inputText);
}

String convertDotsToRealMorse(String dotStream) {
  String result = "";
  int i = 0;
  while (i < dotStream.length()) {
    if (dotStream.substring(i, i + 6) == "......") {
      result += "-";
      i += 6;
    } else {
      result += ".";
      i += 1;
    }  
  }
  return result;
}

void decodeMorseWithSound() {
  String morseChar = "";
  String decodedMessage = "";
  unsigned long lastTapTime = 0;
  const unsigned long charPause = 2000;
  const unsigned long wordPause = 5000;
  bool charDecoded = false;
  bool validLastChar = false;
  bool spaceAdded = false;
  bool decodingEnabled = true;

  Serial.println("\n--- Sound Decoding ---");
  Serial.println("Tap once for a dot and tap six times for a dash. Press # after the end of the message.");
  lcd.clear();
  lcd.print("Decoding...");

  digitalWrite(blueLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);

  while (decodingEnabled) {
    if (digitalRead(soundSensorPin) == HIGH) {
      morseChar += ".";
      lastTapTime = millis();
      charDecoded = false;
      spaceAdded = false;

      Serial.print(".");
      digitalWrite(redLedPin, HIGH);
      delay(200);
      digitalWrite(redLedPin, LOW);
    }

    unsigned long now = millis();
    if (morseChar.length() > 0 && (now - lastTapTime >= charPause) && !charDecoded) {
      Serial.println();
      Serial.print("Raw Dots: ");
      Serial.println(morseChar);

      String realMorse = convertDotsToRealMorse(morseChar);
      char decoded = decodeChar(realMorse);

      Serial.print("Morse: ");
      Serial.print(realMorse);
      Serial.print(" -> Character: ");
      Serial.println(decoded);

      if (decoded != '?') {
        decodedMessage += decoded;
        // validLastChar = true;
      } else {
        Serial.println("[Invalid Morse, ignoring]");
        // validLastChar = false;
      }

      morseChar = "";
      charDecoded = true;
      spaceAdded = false;
    }

    if ((now - lastTapTime >= wordPause) && !spaceAdded && charDecoded) {
      decodedMessage += ' ';
      Serial.println("[Space]");
      spaceAdded = true;
      charDecoded = false;
    }

    char key = keypad.getKey();
    if (key == '#') {
      Serial.println("\n--- Decoding Stopped ---");
      Serial.print("Final Message: ");
      Serial.println(decodedMessage);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Decoded:");
      lcd.setCursor(0, 1);
      lcd.print(decodedMessage);
      decodingEnabled = false;

      for (int i = 0; i < 3; i++) {
        digitalWrite(blueLedPin, HIGH);
        tone(buzzerPin, 1000, 100);
        delay(200);
        digitalWrite(blueLedPin, LOW);
        delay(200);
      }
    }
  }
  delay(3000);
  displayMainMenu();
}

void loop() {
  char key = keypad.getKey();

  if (mode == -1) { // Main menu mode
    if (key == 'A') {
      mode = 0;
      inputText = "";
      digitalWrite(greenLedPin, HIGH);
      Serial.println("\n--- Encode Mode Activated ---");
      lcd.clear();
      lcd.print("Encode Mode");
      delay(1000);
    } else if (key == 'B') {
      mode = 2;
      Serial.println("\n--- Sound Decode Mode Activated ---");
      decodeMorseWithSound();
    }else if (key == 'C') {
      adjustSpeed();
    }else if (key == 'D') {
      mode = 1;
      Serial.println("\n--- Light Decode Mode Activated ---");
      decodeMorseLightSensor();
    }

  } else if (mode == 0) { // Encode mode
    if (key >= '0' && key <= '9') {
      handleT9Input(key);
    } else if (key == '#') {
      Serial.print("Final Message to Encode: ");
      Serial.println(inputText);
      lcd.clear();
      lcd.print("Encoding...");
      encodeMorse(inputText);
    } else if (key == '*') {
      if (inputText.length() > 0) {
        inputText.remove(inputText.length() - 1);
        Serial.print("Typing: ");
        Serial.println(inputText);
        lcd.clear();
        lcd.print("Typing:");
        lcd.setCursor(0, 1);
        lcd.print(inputText);
      }
    }
  }

  // Update speed display continuously only when in speed adjust mode
  if (speedAdjustMode) {
    updateSpeed();
  }
}