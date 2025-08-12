#include <LiquidCrystal.h>


const int MOTOR_BTN        = 2;
const int BELT_BTN         = 3;
const int SEATBELT_LED     = 4;
const int BUZZER_PIN       = 5;
const int MOTOR_CTRL       = 6;
const int TEMP_SENSOR_PIN  = A0;

const int LDR_PIN          = A1;
const int HEADLIGHT_LED    = 13;
const int LDR_THRESHOLD    = 250;

const int FUEL_SENSOR_PIN  = A2;
const int FUEL_LED         = 1;

const int FAN_MOTOR_PIN    = 14;   


const int DOOR_SENSOR_PIN  = 19;
const int DOOR_LED_R       = 17;
const int DOOR_LED_B       = 18;

const float TEMP_THRESHOLD = 25.0;
const int   BASAMAK        = 0;

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
byte degreeChar[8] = { B00111,B00101,B00111,0,0,0,0,0 };


static bool farMessageShown     = false;  // Far mesajı bir kere gösterildi mi

void setup() {
  pinMode(MOTOR_BTN,       INPUT_PULLUP);
  pinMode(BELT_BTN,        INPUT_PULLUP);
  pinMode(SEATBELT_LED,    OUTPUT);
  pinMode(BUZZER_PIN,      OUTPUT);
  pinMode(MOTOR_CTRL,      OUTPUT);

  pinMode(LDR_PIN,         INPUT);
  pinMode(HEADLIGHT_LED,   OUTPUT);

  pinMode(FUEL_SENSOR_PIN, INPUT);
  pinMode(FUEL_LED,        OUTPUT);

  pinMode(FAN_MOTOR_PIN,   OUTPUT);

  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(DOOR_LED_R,      OUTPUT);
  pinMode(DOOR_LED_B,      OUTPUT);

  lcd.begin(16,2);
  lcd.clear();
  lcd.createChar(0, degreeChar);
}


static bool messageActive = false;   // Şu anda LCD'de bir mesaj gösteriliyor mu

void loop() {
  // ---- 5. İSTEK: Kapı Durumu Kontrolü ----
  bool doorOpen = digitalRead(DOOR_SENSOR_PIN) == HIGH;
  if (doorOpen) {
    digitalWrite(DOOR_LED_R, HIGH);
    digitalWrite(DOOR_LED_B, HIGH);
    digitalWrite(MOTOR_CTRL, LOW);          // kapı açıksa motor kesin kapalı
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Uyari: Kapi Acik");
    lcd.setCursor(0,1); lcd.print("Motor Calismaz");
    delay(500);
    return;                                 // diğer kontrolleri atla
  } else {
    digitalWrite(DOOR_LED_R, LOW);
    digitalWrite(DOOR_LED_B, LOW);
    lcd.clear();
  }

  // --------- MOTOR & FAN DURUMLARI ---------
  bool engineOn       = false;  // yalnızca motor için
  //static bool climateOn = false;

  // ---- 1. İSTEK: Emniyet kemeri & motor kontrol ----
  bool motorPressed = digitalRead(MOTOR_BTN) == LOW;
  bool beltOn       = digitalRead(BELT_BTN)  == LOW;

  if (beltOn) {
    digitalWrite(BUZZER_PIN,   LOW);
    digitalWrite(SEATBELT_LED, LOW);
  }

  if (motorPressed) {
    if (!beltOn) {
      // kemer takılı değilse motoru engelle
      digitalWrite(BUZZER_PIN,   HIGH);
      digitalWrite(SEATBELT_LED, HIGH);
      lcd.setCursor(0,0); lcd.print("Emniyet Kemeri");
      lcd.setCursor(0,1); lcd.print("Takili Degil!  ");
      while (digitalRead(MOTOR_BTN)==LOW && digitalRead(BELT_BTN)==HIGH) delay(10);
      if (digitalRead(BELT_BTN)==LOW) {
        digitalWrite(BUZZER_PIN,   LOW);
        digitalWrite(SEATBELT_LED, LOW);
        lcd.clear();
        while (digitalRead(MOTOR_BTN)==LOW) delay(10);
        lcd.clear();
        return;
      }
    } else {
      engineOn = true;                     // motoru calistir
      digitalWrite(BUZZER_PIN,   LOW);
      digitalWrite(SEATBELT_LED, LOW);
      lcd.clear();
    }
  }

// motor çıkışını kapı, kemer ve buton şartlarına göre kontrol et
  digitalWrite(MOTOR_CTRL, engineOn ? HIGH : LOW);  // <<< DEĞİŞTİRİLDİ

  // ---- 2. İSTEK: Sıcaklık ve Klima Kontrolü ----
float voltage = analogRead(TEMP_SENSOR_PIN) * (5.0 / 1023.0);
float tempC   = voltage * 100.0;

if (tempC > TEMP_THRESHOLD && !messageActive) {
    

      messageActive = true;

        lcd.clear();
        lcd.setCursor(0,0); lcd.print("Sicaklik:");
        lcd.setCursor(9,0); lcd.print(tempC);
        lcd.write(byte(0)); lcd.print("C");
        lcd.setCursor(0,1); lcd.print("- Klima Acildi");

        digitalWrite(FAN_MOTOR_PIN, HIGH);  // <<< MOTORU AÇ

        delay(500);
        lcd.clear();
        messageActive = false;
    
}
else if (tempC <= TEMP_THRESHOLD) {
    //climateMessageShown = false;

    digitalWrite(FAN_MOTOR_PIN, LOW);  // <<< MOTORU KAPAT!
}


// ---- 3. İSTEK: Far Kontrolü ----
int countVal = analogRead(LDR_PIN);

int ldrValue = analogRead(LDR_PIN);

// Eğer ortam karanlık  ise far açılır
if (ldrValue <= LDR_THRESHOLD && !messageActive) {
    if (!farMessageShown) {
        farMessageShown = true;
        messageActive = true;

        digitalWrite(HEADLIGHT_LED, HIGH);    // FARI AÇ
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Farlar Acildi");

        delay(1200);
        lcd.clear();
        messageActive = false;
    }
}

// Eğer ortam aydınlık  ise far kapanır
else if (ldrValue > LDR_THRESHOLD && !messageActive) {
    if (farMessageShown) { // Far önceden açıksa şimdi kapanacak
        farMessageShown = false;
        messageActive = true;

        digitalWrite(HEADLIGHT_LED, LOW);     // FARI KAPAT
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Farlar Kapandi");

        delay(1200);
        lcd.clear();
        messageActive = false;
    }
}


  // ---- 4. İSTEK: Yakıt Seviyesi Uyarı & Kontrol ----
  static unsigned long lastBlink = 0;
  static bool        blinkOn    = false;
  int   fuelRaw = analogRead(FUEL_SENSOR_PIN);
  float fuelPct = fuelRaw * 100.0 / 1023.0;

  if (fuelPct <= 0.0) {
    digitalWrite(FUEL_LED,      LOW);
    digitalWrite(MOTOR_CTRL,    LOW);
    digitalWrite(HEADLIGHT_LED, LOW);
    digitalWrite(SEATBELT_LED,  LOW);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Yakit Bitti     ");
    lcd.setCursor(0,1); lcd.print("Motor Durdu     ");
    delay(1000);
   
  }
  else if (fuelPct < 5.0) {
    if (millis() - lastBlink >= 500) {
      lastBlink = millis();
      blinkOn = !blinkOn;
      digitalWrite(FUEL_LED, blinkOn ? HIGH : LOW);
    }
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Kritik: Yakit  ");
    lcd.setCursor(0,1); lcd.print("Cok Az %"); lcd.print(fuelPct);
    delay(500);
  }
  else if (fuelPct < 10.0) {
    digitalWrite(FUEL_LED, HIGH);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(" Yakit Seviyesi");
    lcd.setCursor(0,1); lcd.print(" Dusuk %"); lcd.print(fuelPct);
    delay(500);
  }
  else {
    digitalWrite(FUEL_LED, LOW);
    lcd.clear();
  }
}