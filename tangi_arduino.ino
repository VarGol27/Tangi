#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <time.h>
#include <pgmspace.h>

const int buttonPin = 25;
const int switchPin = 34;
const int motionPin = 26;
const int buzzerPin = 4;

// BLE UUIDs
#define BLE_DEVICE_NAME "ESP32_Feather_BLE"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_C4  262

// Melodies
const int melody[] PROGMEM = {NOTE_C3, NOTE_E3, NOTE_G3, NOTE_F3, NOTE_D3, NOTE_C4};
const int shortMelody[] PROGMEM = {NOTE_C3, NOTE_E3, NOTE_D3};
const int noteDurations[] PROGMEM = {6, 6, 6, 6, 6, 6}; 
const int shortNoteDurations[] PROGMEM = {8, 8, 8};

// Timer variables
unsigned long timerEndTime = 0;
bool buzzerActive = false;
bool timerRunning = false;
bool timerStarted = false;
struct tm targetTime;
bool motionDetected = false;

BLEServer *pServer = nullptr;
BLEService *pService = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
BLEAdvertising *pAdvertising = nullptr;
uint16_t connHandle = 0;

// Function to set compile time
void setCompileTime() {
  struct tm timeInfo;
  sscanf(__DATE__, "%*s %d %d", &timeInfo.tm_mday, &timeInfo.tm_year);
  sscanf(__TIME__, "%d:%d:%d", &timeInfo.tm_hour, &timeInfo.tm_min, &timeInfo.tm_sec);

  String monthStr = String(__DATE__).substring(0, 3);
  timeInfo.tm_mon = (monthStr == "Jan") ? 0 : (monthStr == "Feb") ? 1 : (monthStr == "Mar") ? 2 :
                    (monthStr == "Apr") ? 3 : (monthStr == "May") ? 4 : (monthStr == "Jun") ? 5 :
                    (monthStr == "Jul") ? 6 : (monthStr == "Aug") ? 7 : (monthStr == "Sep") ? 8 :
                    (monthStr == "Oct") ? 9 : (monthStr == "Nov") ? 10 : 11;

  timeInfo.tm_year -= 1900;
  timeInfo.tm_isdst = -1;
  time_t epochTime = mktime(&timeInfo);
  struct timeval tv = {epochTime, 0};
  settimeofday(&tv, nullptr);
}

// Function to set current time
void setCurrentTime(int year, int month, int day, int hour, int minute) {
  struct tm timeInfo = {0};
  timeInfo.tm_year = year - 1900;
  timeInfo.tm_mon = month - 1;
  timeInfo.tm_mday = day;
  timeInfo.tm_hour = hour;
  timeInfo.tm_min = minute;
  time_t epochTime = mktime(&timeInfo);
  struct timeval tv = {epochTime, 0};
  settimeofday(&tv, nullptr);  // Set the time
}


// Set target time for the timer
void setTargetTime(String dateTimeStr) {
  int year, month, day, hour, minute;
  sscanf(dateTimeStr.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
  
  targetTime = {0};
  targetTime.tm_year = year - 1900;
  targetTime.tm_mon = month - 1;
  targetTime.tm_mday = day;
  targetTime.tm_hour = hour;
  targetTime.tm_min = minute;
  
  time_t epochTime = mktime(&targetTime);
  long timeDifferenceSec = epochTime - time(NULL); // The difference between current time and target time
  if (timeDifferenceSec > 0) {  
    timerEndTime = time(NULL) + timeDifferenceSec;
    timerRunning = true;
    timerStarted = false;
  }
}


// BLE Server Callbacks to handle connection
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    connHandle = server->getConnId();
    Serial.println("Client connected.");
  }

  void onDisconnect(BLEServer *server) override {
    Serial.println("Client disconnected.");
    BLEDevice::startAdvertising();
  }
};

// BLE Characteristic Callbacks to handle writing
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue().c_str();
    if (value.startsWith("settime ")) {
      int year, month, day, hour, minute;
      sscanf(value.c_str(), "settime %d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
      setCurrentTime(year, month, day, hour, minute);
    } else if (value.startsWith("targettime ")) {
      setTargetTime(value.substring(11));
    }
  }
};


// Initialize BLE
void initializeBLE() {
  BLEDevice::init(BLE_DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

void playMelody() {
  for (int i = 0; i < 6; i++) {
    int noteDuration = 1000 / pgm_read_word_near(noteDurations + i);
    tone(buzzerPin, pgm_read_word_near(melody + i), noteDuration);
    delay(noteDuration * 1.2);
    noTone(buzzerPin);
  }
}

void playShortMelody() {
  for (int i = 0; i < 3; i++) {
    int noteDuration = 1000 / pgm_read_word_near(shortNoteDurations + i);
    tone(buzzerPin, pgm_read_word_near(shortMelody + i), noteDuration);
    delay(noteDuration * 1.1);
    noTone(buzzerPin);
  }
}

void setup() {
  Serial.begin(115200);
  setCompileTime();
  pinMode(buttonPin, INPUT);
  pinMode(switchPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  initializeBLE();
}

void loop() {
  time_t currentEpochTime = time(NULL);

  if (digitalRead(switchPin) == HIGH) {
    noTone(buzzerPin);
    buzzerActive = false;
    timerStarted = false;
    motionDetected = false;
    return;
  }

  if (timerRunning && !timerStarted && digitalRead(switchPin) == LOW) {
    timerStarted = true;
  }

  if (timerStarted && currentEpochTime >= timerEndTime && !buzzerActive) {
    buzzerActive = true;
    timerRunning = false;
  }

  while (buzzerActive) {
    playMelody();

    if (digitalRead(switchPin) == HIGH) {
      noTone(buzzerPin);
      buzzerActive = false;
      break;
    }

    if (digitalRead(buttonPin) == HIGH) {
      noTone(buzzerPin);      
      buzzerActive = false;    

      timerEndTime = time(NULL) + 600;
      timerRunning = true;    
      timerStarted = false;  
      delay(500);        
      break;
    }
  }

  if (digitalRead(motionPin) == HIGH && !motionDetected && !buzzerActive) {
    motionDetected = true;
    playShortMelody();
  }

  if (digitalRead(motionPin) == LOW) {
    motionDetected = false;
  }

  delay(100); 
}

