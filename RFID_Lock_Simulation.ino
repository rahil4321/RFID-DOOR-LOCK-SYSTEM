#include <Servo.h>
#include <EEPROM.h>

Servo lockServo;

const int CARD_BUTTON = 2;
const int ADMIN_BUTTON = 7;
const int SERVO_PIN = 6;
const int RED_LED = 4;
const int GREEN_LED = 5;
const int BUZZER = 3;

const int MAX_USERS = 10;
const int MAX_FAILURES = 3;

int users[MAX_USERS];
int userCount = 1;
int nextCardID = 2; 

int failures = 0;
bool lockedOut = false;
bool doorOpen = false;
bool adminMode = false;

unsigned long lockoutStart = 0;
unsigned long unlockStart = 0;

const unsigned long LOCKOUT_TIME = 15000;
const unsigned long UNLOCK_TIME = 3000;

bool previousCard = HIGH;
bool previousAdmin = HIGH;

void logEvent(int id, const char *result) {
  Serial.print("Card ID: ");
  Serial.print(id);
  Serial.print(" | Result: ");
  Serial.println(result);
}

bool registered(int id) {
  for (int i = 0; i < userCount; i++) {
    if (users[i] == id) return true;
  }
  return false;
}

void unlockDoor() {
  Serial.println("ACCESS GRANTED");
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  tone(BUZZER, 2000, 150);
  lockServo.write(90);
  doorOpen = true;
  unlockStart = millis();
}

void denyAccess(int id) {
  logEvent(id, "DENIED");
  Serial.println("ACCESS DENIED");

  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  tone(BUZZER, 500, 400);

  failures++;

  if (failures >= MAX_FAILURES) {
    lockedOut = true;
    lockoutStart = millis();
    Serial.println("LOCKOUT: Wait 15 seconds.");
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(CARD_BUTTON, INPUT_PULLUP);
  pinMode(ADMIN_BUTTON, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lockServo.attach(SERVO_PIN);
  lockServo.write(0);


  users[0] = 1;

  Serial.println("RFID LOCK SIMULATION READY");
  Serial.println("Button D2 = simulate card scan (Scans ID 1 in Normal Mode)");
  Serial.println("Button D7 = admin mode");
  Serial.println("Initial registered card ID: 1");
}

void loop() {
  bool cardNow = digitalRead(CARD_BUTTON);
  bool adminNow = digitalRead(ADMIN_BUTTON);

  if (doorOpen && millis() - unlockStart >= UNLOCK_TIME) {
    lockServo.write(0);
    doorOpen = false;
    digitalWrite(GREEN_LED, LOW);
    Serial.println("Door relocked.");
  }

  if (lockedOut) {
    if (millis() - lockoutStart >= LOCKOUT_TIME) {
      lockedOut = false;
      failures = 0;
      digitalWrite(RED_LED, LOW);
      Serial.println("Lockout ended.");
    } else {
      previousCard = cardNow;
      previousAdmin = adminNow;
      return;
    }
  }


  if (previousAdmin == HIGH && adminNow == LOW) {
    adminMode = !adminMode;

    if (adminMode) {
      Serial.println("ADMIN MODE ON");
      Serial.println("D2: add next card ID");
      Serial.println("D7: exit admin mode");
    } else {
      Serial.println("ADMIN MODE OFF");
    }

    delay(200);
  }


  if (previousCard == HIGH && cardNow == LOW) {
    if (adminMode) {
      if (userCount < MAX_USERS) {
        int id = nextCardID++;
        users[userCount++] = id;

        Serial.print("Registered simulated card ID: ");
        Serial.println(id);
      } else {
        Serial.println("User list full.");
      }
    } else {
      int id = 1; 
      logEvent(id, registered(id) ? "GRANTED" : "DENIED");

      if (registered(id)) {
        failures = 0;
        unlockDoor();
      } else {
        denyAccess(id);
      }
    }

    delay(200);
  }


  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'C' || cmd == 'c') {
      if (registered(1)) {
        failures = 0;
        Serial.println("AUTHORIZED CARD DETECTED");
        logEvent(1, "GRANTED");
        unlockDoor();
      } else {
        Serial.println("Card 1 is not registered!");
        denyAccess(1);
      }
    }

    if (cmd == 'U' || cmd == 'u') {
      denyAccess(99);
    }

    if (cmd == 'R' || cmd == 'r') {
      for (int i = 0; i < userCount; i++) {
        if (users[i] == 1) {
          for (int j = i; j < userCount - 1; j++) {
            users[j] = users[j + 1];
          }
          users[userCount - 1] = 0; 
          userCount--;
          Serial.println("Card ID 1 revoked.");
          break;
        }
      }
    }
  }

  previousCard = cardNow;
  previousAdmin = adminNow;
}
