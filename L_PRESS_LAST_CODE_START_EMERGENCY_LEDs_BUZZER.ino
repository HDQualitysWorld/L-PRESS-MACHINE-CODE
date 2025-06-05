#include <Servo.h>

const int buttonPin = 2; //START BUTONU PİNİ
const int emergencyPin = 3; // EMERGENCY STOP BUTONU PİNİ 
const int relay1 = 4;  // BÜKÜM PİSTONU
const int relay2 = 5;  // KUTUYA ATMA PİSTONU


const int redLED = 12;    // EMERGENCY LED
const int greenLED = 11;  // START LED
const int buzzer = 7;      //SİSTEM İLK AÇILDIĞINDA ÇALAN BUZZER

Servo servo1; //SERVONUN ÜSTÜNDE S1 YAZAR (ÜSTTEN BAKINCA SAĞDAKİ MAKİNE DIŞINDAKİ SERVO)
Servo servo2;

bool systemActive = false;
bool buttonPressed = false;
unsigned long pressStartTime = 0;
unsigned long lastMillis = 0;

void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(emergencyPin, INPUT_PULLUP);

  pinMode(relay1, OUTPUT);  // BÜKÜM PİSTON VALFİ RÖLESİ
  pinMode(relay2, OUTPUT);  // KUTUYA ATMA PİSTON VALFİ RÖLESİ
  

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(buzzer, OUTPUT);

  servo1.attach(9);  //SERVOLARIN PİN TANIMLAMALARI
  servo2.attach(10);
  

   // ENERJİ VERİLDİĞİ ANDA SİSTEMDE BİR KEZ OKUNACAK KODLAR (BAŞLANGIÇ DURUMU)!!
  servo1.write(177);    //SERVO 1 NORMAL KONUMU
  servo2.write(3);      //SERV0 2 NORMAL KONUMU

  digitalWrite(relay1, HIGH);  
  digitalWrite(relay2, HIGH);  // Rölerlerin HIGH olmaı rölenin boşta oldğu anlamına gelir. LOW yazarsan röleye akım gider be tetikleme yapılır.!!!!!

  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
  


  Serial.println("System started. Waiting for button press...");
}

void loop() {

  // Emergency kontrolü her döngüde ilk yapılır
  if (digitalRead(emergencyPin) == LOW) {
    handleEmergency();
    return;
  }

  digitalWrite(redLED, LOW); // Emergency değilse kırmızı LED sönsün

  if (!systemActive) {
    // Hazır modda yeşil LED 500ms de bir yanıp sönsün
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 500) {    // Hazırda bekleme ledinin (greenled) yanıp sönme sıklığı 500ms. (green led blink delay)
      lastMillis = currentMillis;
      digitalWrite(greenLED, !digitalRead(greenLED));
    }
  } else {
    digitalWrite(greenLED, HIGH); // starta basıldığında ve döngü çalışıyorsa greenLED sabit yansın
  }

  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW && !buttonPressed) {
    pressStartTime = millis();
    buttonPressed = true;
  }

  if (buttonState == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - pressStartTime;
    buttonPressed = false;

    if (pressDuration >= 100 && !systemActive) {
      Serial.println("Button pressed. Starting sequence.");
      systemActive = true;
      digitalWrite(greenLED, HIGH);

      // DÖNGÜ BAŞLANGICI
      if (!checkEmergencyDuringAction()) return;

      // Servo hareketi İLER - BEKLEME - GERİ
     // DİKKAT: SERVOLAR KARŞILIKLI KONULDUĞU İÇİN, VOID SETUP İÇİNDE YAZAN İLK KONUMA GÖRE AÇILARI BİRİRİNE ZIT OLARAK DEĞİŞTİRİLECEKTİR. AŞAĞIDAKİ KOD SATIRINDA SÜRME MEKANİZMASI İLERİ HAREKET EDER.

      servo1.write(8);         //SERVOLAR İLERİ HAREKET ETTİ, SAC V YATAĞINA SÜRÜLDÜ VE 3000ms BEKLENDİ
      servo2.write(172);
      if (!waitWithEmergency(3000)) return;   //SERVO hareket süresi delay yani.

      servo1.write(177); //SERVOLAR GERİ KONUMA ÇEKİLDİ VE 1000ms SONRA DİĞER SATIRA GEÇİLDİ
      servo2.write(3);
      if (!waitWithEmergency(1000)) return;


      digitalWrite(relay1, LOW);   // BÜKÜM PİSTONU VALFİ AKTİF. (BÜKÜM PİSTONU SACI BÜKTÜ)
      digitalWrite(relay2, HIGH);
      if (!waitWithEmergency(1500)) return; // 1500ms Büküm konumunda kal 

  
      digitalWrite(relay1, HIGH); // BÜKÜM PİSTONUNU GERİ ÇEK
      digitalWrite(relay2, HIGH);
       if (!waitWithEmergency(1500)) return;
     
//------------KUTUYA ATMA DÖNGÜSÜ-----------

     
      digitalWrite(relay1, HIGH);
      digitalWrite(relay2, LOW);  // Kutuya atan piston valfi aktif
      if (!waitWithEmergency(1500)) return;

      
      digitalWrite(relay1, HIGH);
      digitalWrite(relay2, HIGH); // kutuya atan piston valfi kapalı
      if (!waitWithEmergency(1000)) return;

      Serial.println("Process complete.");
      systemActive = false;
      lastMillis = millis(); // Yanıp sönme
    }
  }
}

// EMERGENCY STOP DURUMUNUN DÖNGÜNÜN HER YERİNDEN ÇAĞIRABİLECEĞİN YER!!
// EMERGENCY DURUMUNDA RÖLELER VE SERVO KONUMLARI: VOİD handleEMERGENCY içerisinde yazan gibi olacaktır. (SERVOLAR İLK KONUMA GİDECEK VE TÜM PİSTONLAR GERİ ÇEKİLECEK)
void handleEmergency() {
  Serial.println("!!! EMERGENCY !!! System in SAFE MODE");

  servo1.write(177);
  servo2.write(3);

  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  //digitalWrite(relay3, HIGH);
 //digitalWrite(relay4, HIGH);

  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, LOW);

  systemActive = false;
  delay(500); // Yanıp söner gibi görünüm ama sistemin çalışmadığı durum
}

// Döngüdeyken sürekli Emergency kontrolü
bool checkEmergencyDuringAction() {
  if (digitalRead(emergencyPin) == LOW) {
    handleEmergency();
    return false;
  }
  return true;
}

// Delay yerine kullanılan, arada emergency kontrolü yapan özel bekleme fonksiyonumuz
bool waitWithEmergency(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (!checkEmergencyDuringAction()) return false;
    delay(10);
  }
  return true;
}

