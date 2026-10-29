#define BLYNK_TEMPLATE_ID "TMPL3DZO778SZ"
#define BLYNK_TEMPLATE_NAME "SMART LOCK"
#define BLYNK_AUTH_TOKEN "ZC33lsD18UrzCi5ik3DBFCL3oinIFlW2"

#define BLYNK_PRINT Serial
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <Keypad.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// 🔹 Wi-Fi & Blynk Setup
char auth[] = "ZC33lsD18UrzCi5ik3DBFCL3oinIFlW2"; 
char ssid[] = "Sudhikxx";
char pass[] = "Ksudhi14";

// 🔹 LCD Setup
LiquidCrystal_PCF8574 lcd(0x27);

// 🔹 Keypad Setup
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = { {'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'} };
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// 🔹 GPIO Assignments
#define RELAY_PIN 4
#define HALL_SENSOR_PIN 19
#define BUZZER_PIN 5
#define TAMPER_SENSOR_PIN 18  

// 🔹 Password Setup
String password = "1234";
String inputPassword = "";
bool doorUnlocked = false;

// 🔹 Timer Setup
unsigned long lockOpenStartTime = 0;
const unsigned long lockOpenTimeout = 30000; // 30 seconds alert timeout

// 🔹 Tamper Detection Variables
bool lockInProgress = false;
unsigned long lastTamperTime = 0;
const unsigned long tamperThreshold = 1000; 

// 🔹 Tamper Cooldown Variables
bool tamperCooldown = false;
const unsigned long tamperCooldownTime = 3000;
unsigned long tamperCooldownStart = 0;

void setup() {
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);

    // 🔹 GPIO Setup
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH); // Locked initially
    pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    pinMode(TAMPER_SENSOR_PIN, INPUT_PULLUP);

    // 🔹 LCD Setup
    Wire.begin(21, 22);
    lcd.begin(16, 2);
    lcd.setBacklight(1);
    updateLCD("Enter Password", "");
}

void loop() {
    Blynk.run();  
    handleKeypadInput();
    detectTampering();
    checkLockOpenTooLong();
}

// 🔹 Handle Keypad Input
void handleKeypadInput() {
    char key = keypad.getKey();
    if (key) {
        if (key == '#') {  
            inputPassword = "";
            updateLCD("Enter Password", "");
        } else if (key == '*') {  
            if (inputPassword == password) {
                updateLCD("Access Granted", "");
                unlockDoor("Keypad");
            } else {
                updateLCD("Access Denied", "");
                Blynk.logEvent("wrong_password", "❌ Wrong password attempt!");
                delay(2000);
                updateLCD("Enter Password", "");
            }
            inputPassword = "";
        } else if (key == 'D') {  
            lockDoor("Keypad");
        } else {
            if (inputPassword.length() < 6) {
                inputPassword += key;
                updateLCD("Enter Password", maskPassword(inputPassword));
            }
        }
    }
}

// 🔹 Unlock Door
void unlockDoor(String source) {
    digitalWrite(RELAY_PIN, LOW);
    Blynk.virtualWrite(V1, "🔓 Unlocked");
    Blynk.logEvent("lock_status", "🔓 Lock opened via " + source);
    doorUnlocked = true;
    lockOpenStartTime = millis();
    
    // Start cooldown for tamper detection
    tamperCooldown = true;
    tamperCooldownStart = millis();
}

// 🔹 Lock Door
void lockDoor(String source) {
    lockInProgress = true;
    digitalWrite(RELAY_PIN, HIGH);
    Blynk.virtualWrite(V1, "🔒 Locked");
    Blynk.logEvent("lock_status", "🔒 Lock closed via " + source);
    delay(500);
    lockInProgress = false;

    // Start cooldown for tamper detection
    tamperCooldown = true;
    tamperCooldownStart = millis();
    updateLCD("Enter Password", "");
}

// 🔹 Detect Tampering
void detectTampering() {
    static int lastState = digitalRead(TAMPER_SENSOR_PIN);
    int currentState = digitalRead(TAMPER_SENSOR_PIN);
    unsigned long currentTime = millis();

    if (tamperCooldown && (currentTime - tamperCooldownStart < tamperCooldownTime)) {
        return;
    } else {
        tamperCooldown = false;
    }

    if (lockInProgress) {
        lastState = currentState;
        return;
    }

    if (currentState != lastState && currentState == LOW) {
        if (currentTime - lastTamperTime > tamperThreshold) {
            Serial.println("🚨 Tampering Detected!");
            Blynk.logEvent("tamper_alert", "🚨 Tamper detected!");
            for (int i = 0; i < 6; i++) {  
                digitalWrite(BUZZER_PIN, HIGH);
                delay(250);
                digitalWrite(BUZZER_PIN, LOW);
                delay(250);
            }
        }
        lastTamperTime = currentTime;
    }
    lastState = currentState;
}

// 🔹 Check if Lock is Open Too Long
void checkLockOpenTooLong() {
    if (doorUnlocked && (millis() - lockOpenStartTime > lockOpenTimeout)) {
        Serial.println("⚠️ Lock Open Too Long!");
        Blynk.logEvent("lock_open_alert", "⚠️ The lock has been left open for too long!");
        doorUnlocked = false;
        lockDoor("Auto");
    }
}

// 🔹 Mask Password
String maskPassword(String input) {
    String masked = "";
    for (int i = 0; i < input.length(); i++) {
        masked += '*';
    }
    return masked;
}

// 🔹 Update LCD
void updateLCD(String line1, String line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

// 🔹 Blynk App Button Handling
BLYNK_WRITE(V0) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        unlockDoor("Blynk App");
    } else {
        lockDoor("Blynk App");
    }
}
