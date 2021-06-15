//#define CAYENNE_DEBUG       // Uncomment to show debug messages
//#define CAYENNE_PRINT Serial  // Comment this out to disable prints and save space
#include <CayenneMQTTEthernet.h>

#include <DHT.h>

#include <HX711_ADC.h>
const int HX711_dout = 9; //mcu > HX711 dout pin
const int HX711_sck = 8; //mcu > HX711 sck pin
//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
unsigned long t = 0;

//Sensor de luz
int ledPin = 5; // Piezo on Pin 5
int ldrPin = 0; // LDR en el pin analogico 0
int ldrValue = 0;

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 6
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "8fb16150-b506-11eb-8779-7d56e82df461";
char password[] = "7db53d0d3274a708b1567c95e804a23ae0064798";
char clientID[] = "31f6a130-bad1-11eb-8779-7d56e82df461";
//pin del ventilador
const int ventilador = 7;
 
void setup() {
	Serial.begin(57600);
  //Balanza
  Serial.println();
  Serial.println("Starting...");
  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = -122.26; // uncomment this if you want to set the calibration value in the sketch
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  //Sensor de luz
   pinMode(ledPin,OUTPUT);
  // Comenzamos el sensor DHT
  dht.begin();
  //definicion pin ventilador
  pinMode(ventilador, OUTPUT);
  // Comenzamos cayenne
	Cayenne.begin(username, password, clientID);
}

void loop() {
  //Sensor de luz
  ldrValue = analogRead(ldrPin); 
  //Serial.println(ldrValue);
  if (ldrValue >= 1023){
    digitalWrite(ledPin,HIGH);
  }
  else {
    digitalWrite(ledPin,LOW);
  }
  
  //Balanza
static boolean newDataReady = 0;
float peso;
  const int serialPrintInterval = 100; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      peso = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(peso);
      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
    }
  
  // Leemos la humedad relativa
  float humedad = dht.readHumidity();
  // Leemos la temperatura en grados centígrados
  float temperatura = dht.readTemperature();
  // Calcular el índice de calor en grados centígrados
  float indicecalor = dht.computeHeatIndex(temperatura, humedad, false);
  
  //inicia cayenne
	Cayenne.loop();
  Cayenne.virtualWrite(0, humedad);
  Cayenne.virtualWrite(1, temperatura);
  Cayenne.virtualWrite(2, indicecalor);
  Cayenne.virtualWrite(3, peso);
  Cayenne.virtualWrite(4, ldrValue);
// Esperamos 5 segundos entre medidas
  delay(5000);
}
CAYENNE_IN(5)
{
  int Valor_Canal_05 = getValue.asInt();
  digitalWrite(ventilador, Valor_Canal_05);
}
