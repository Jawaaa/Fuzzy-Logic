#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h> // library untuk sensor temperatur

#define ONE_WIRE_BUS 2
#define RELAY_PIN 8 // mendefinisikan pin 8 sebagai pin yang berhubungan dengan relay
#define WATERLEVEL_PIN A0 // mendefinisikan pin A4 sebagai pin yang berhubungan dengan sensor water level

// Fuzzy Variables
float waterLevelValue; // input water level
float temperature; // input sensor temperature
float pumpDuration; // Output durasi nyalanya pompa (dalam detik)

//Trigger sensor temperatur DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fuzzy Sets
float cold(float x) {
  if (x < 20) return 1;
  if (x > 25) return 0;
  return (25 - x) / (25 - 20);
}

float normal(float x) {
  if (x < 20 || x > 35) return 0;
  if (x >= 20 && x <= 25) return (x - 20) / (25 - 20);
  if (x >= 30 && x <= 35) return (35 - x) / (35 - 30);
  return 1;
}

float hot(float x) {
  if (x <= 30) return 0;
  if (x >= 35) return 1;
  return (x - 30) / (35 - 30);
}

float low(float x) {
  if (x < 1) return 1;
  if (x >= 4) return 0;
  if (x > 1 && x < 4) return (4 - x) / (4 - 1);
  return 0;
}

float medium(float x) {
  if (x < 1 || x >= 4) return 0;
  if (x >= 1 && x < 2) return (x - 1) / (2 - 1);
  if (x >= 2 && x <= 3) return 1;
  if (x > 3 && x < 4) return (4 - x) / (4 - 3);
  return 0;
}

float high(float x) {
  if (x >= 4) return 1;
  if (x <= 1) return 0;
  if (x > 1 && x < 4) return (x - 1) / (4 - 1);
  return 0;
}



// Fuzzy Rules
float rule1(float temp, float level) {
  // Rule 1: Suhu Dingin dan Level Air Rendah
  // Jika suhu dingin dan level air rendah, maka pompa menyala selama 3 detik.
  float tempValue = cold(temp);
  float levelValue = low(level);
  float output = min(tempValue, levelValue) * 3;
  return output;
}

float rule2(float temp, float level) {
  // Rule 2: Suhu Dingin dan Level Air Medium
  // Jika suhu dingin dan level air medium, maka pompa menyala selama 5 detik.
  float tempValue = cold(temp);
  float levelValue = medium(level);
  float output = min(tempValue, levelValue) * 5;
  return output;
}

float rule3(float temp, float level) {
  // Rule 3: Suhu Dingin dan Level Air Tinggi
  // Jika suhu dingin dan level air tinggi, maka pompa menyala selama 7 detik.
  float tempValue = cold(temp);
  float levelValue = high(level);
  float output = min(tempValue, levelValue) * 7;
  return output;
}

float rule4(float temp, float level) {
  // Rule 4: Suhu Normal dan Level Air Rendah
  // Jika suhu normal dan level air rendah, maka pompa menyala selama 5 detik.
  float tempValue = normal(temp);
  float levelValue = low(level);
  float output = min(tempValue, levelValue) * 5;
  return output;
}

float rule5(float temp, float level) {
  // Rule 5: Suhu Normal dan Level Air Medium
  // Jika suhu normal dan level air medium, maka pompa menyala selama 7 detik.
  float tempValue = normal(temp);
  float levelValue = medium(level);
  float output = min(tempValue, levelValue) * 7;
  return output;
}

float rule6(float temp, float level) {
  // Rule 6: Suhu Normal dan Level Air Tinggi
  // Jika suhu normal dan level air tinggi, maka pompa menyala selama 10 detik.
  float tempValue = normal(temp);
  float levelValue = high(level);
  float output = min(tempValue, levelValue) * 10;
  return output;
}

float rule7(float temp, float level) {
  // Rule 7: Suhu Panas dan Level Air Rendah
  // Jika suhu panas dan level air rendah, maka pompa menyala selama 7 detik.
  float tempValue = hot(temp);
  float levelValue = low(level);
  float output = min(tempValue, levelValue) * 7;
  return output;
}

float rule8(float temp, float level) {
  // Rule 8: Suhu Panas dan Level Air Medium
  // Jika suhu panas dan level air medium, maka pompa menyala selama 10 detik.
  float tempValue = hot(temp);
  float levelValue = medium(level);
  float output = min(tempValue, levelValue) * 10;
  return output;
}

float rule9(float temp, float level) {
  // Rule 9: Suhu Panas dan Level Air Tinggi
  // Jika suhu panas dan level air tinggi, maka pompa menyala selama 15 detik.
  float tempValue = hot(temp);
  float levelValue = high(level);
  float output = min(tempValue, levelValue) * 15;
  return output;
}

float fuzzyInference(float suhu, float waterlevelValue) {
  // Evaluasi setiap rule berdasarkan kondisi suhu dan kelembaban
  // dan tentukan durasi nyalanya pompa berdasarkan rule yang memenuhi.
  float output = 0;

  output = max(output, rule1(suhu, waterlevelValue));
  output = max(output, rule2(suhu, waterlevelValue));
  output = max(output, rule3(suhu, waterlevelValue));
  output = max(output, rule4(suhu, waterlevelValue));
  output = max(output, rule5(suhu, waterlevelValue));
  output = max(output, rule6(suhu, waterlevelValue));
  output = max(output, rule7(suhu, waterlevelValue));
  output = max(output, rule8(suhu, waterlevelValue));
  output = max(output, rule9(suhu, waterlevelValue));

  return output;
}

// Setup
void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  lcd.begin(16, 2);
}


// Loop
void loop() {
  // Baca suhu dari sensor DS18B20
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0); // Ubah menjadi variabel global

  // Baca level air dari sensor
  int sensorValue = analogRead(WATERLEVEL_PIN);
  // Konversi nilai sensor menjadi level air dalam cm
  waterLevelValue = map(sensorValue, 0, 1023, 0, 5); // Ubah menjadi variabel global

  // Hitung durasi pompa menggunakan fuzzy inference
  pumpDuration = fuzzyInference(temperature, waterLevelValue);

  // Nyalakan pompa selama durasi yang ditentukan
  digitalWrite(RELAY_PIN, HIGH);
  delay(pumpDuration * 1000);
  digitalWrite(RELAY_PIN, LOW);

  // Tampilkan nilai suhu, durasi pompa, dan level air pada Serial Monitor
  Serial.print("Suhu (C): ");
  Serial.println(temperature);
  Serial.print("Durasi Pompa (detik): ");
  Serial.println(pumpDuration);
  Serial.print("Level Air (cm): ");
  Serial.println(waterLevelValue);

  // Tampilkan nilai suhu dan durasi pompa pada LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Suhu (C):");
  lcd.println(temperature);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Level Air (cm):");
  lcd.println(waterLevelValue);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("pump(detik):");
  lcd.println(pumpDuration);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  delay(1000); // Delay 1 detik sebelum membaca data sensor lagi
}