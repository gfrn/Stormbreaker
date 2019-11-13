/* Código Stormbreaker - Grupo 1

    Bibliotecas necessárias implícitas:
    Adafruit Unified Sensor (RTClib e DHT dependem desta)

    Utilização de memória (UNO): 64%
*/

#include <DHT.h> //Instalar AUS

#include <SD.h>

#include "RTClib.h" //Instalar AUS

RTC_DS1307 rtc;

const int LDR = A0;
const int DHTP = A1;
const int UV = A2;
const int REED = 2; //Responsável pela medição de RPM da ventoinha conectada ao anemômetro
const int LED = 3; //LED I/O SD em operação
const int SDCS = 10; //Pino Chip Select do módulo SD
const int DHTTYPE = DHT11;

DHT dht(DHTP, DHTTYPE);

File logFile; //Arquivo em que serão guardadas as informações

float UV_index = 0;
float humidity = 0;
float temp = 0;
float RPM = 0;
float luminosity = 0;

int interval = 1; //Intervalo em minutos
int lastMinute = 0;

int pulses = 0;
bool pulseSwitched = 0; //True enquanto o sensor reed está HIGH depois de incrementar o número de pulsos para evitar falsos positivos de rotação. Reseta em LOW.

void setup() {
  pinMode(LDR, INPUT);
  pinMode(DHTP, INPUT);
  pinMode(UV, INPUT);
  pinMode(REED, INPUT);
  pinMode(LED, OUTPUT);
  
  digitalWrite(LED, LOW);

  Serial.begin(9600);

  if (!SD.begin(SDCS)) {
    Serial.println("Erro inicializando o cartao!");
  } else {
    Serial.println("Cartao Inicializado");
    digitalWrite(LED_BUILTIN, HIGH);
  }

  rtc.begin();

  dht.begin();
}

void loop() {

    if (Serial.available()) {
    String valueSerial = Serial.readString();
    char interpret = valueSerial.charAt(0);

    switch (interpret) {
    case 'd':
      //Computador envia sinal pedindo mudança de horário salvo do RTC        
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

      Serial.print("Novo horario estabelecido");
      break;
    case 'l':
      //Computador pede a transferência das informações no log salvo no cartão SD
      logFile = SD.open("log.txt", FILE_READ);

      if (!logFile) {
        Serial.println("Erro na abertura do arquivo!");
      } else {
        delay(3);
        Serial.println("Leitura do SD:");
        String buffer = "";
        while (logFile.available()) {
          //Lê até primeiro carriage return, escreve linha. Maior estabilidade do que readString em testes
          buffer = logFile.readStringUntil('\n');
          Serial.println(buffer);
        }

        Serial.println("-----------------------------------");
      }
      break;
    }
  } else {
    digitalWrite(LED, HIGH);

    //Realiza leitura de sensores e tradução de valores
    UV_index = analogRead(UV) - 13;
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    luminosity = analogRead(LDR);

    DateTime now = rtc.now();
    //A diferença do último valor de minuto guardado tem que ser maior que o intervalo estabelecido pelo usuário
    if (now.minute() - lastMinute >= interval || lastMinute > now.minute()) {
      logFile = SD.open("log.txt", FILE_WRITE);

      //LED mostra se o cartão SD está em estado de leitura/escrita para impedir que o usuário desligue o Arduino e corrompa o arquivo
      digitalWrite(LED, HIGH);
      RPM = pulses;
      pulses = 0;

      //String de buffer para parsing tem que ser declarada a cada iteração do loop
      char format[] = "DD/MM/YYYY - hh:mm:ss";
      
      //Escrever desta forma economiza recursos e memória do Arduino em comparação a concatenação de strings/objetos por operador + adicionada em novas versões
      Serial.print(now.toString(format));
      Serial.print("|");
      Serial.print(temp);
      Serial.print("|");
      Serial.print(humidity);
      Serial.print("|");
      Serial.print(RPM);
      Serial.print("|");
      Serial.print(UV_index);
      Serial.print("|");
      Serial.println(luminosity);
      if (!logFile.println()) {
        Serial.println("Erro na abertura do arquivo!");
      } else {
        logFile.print(now.toString(format));
        logFile.print("|");
        logFile.print(temp);
        logFile.print("|");
        logFile.print(humidity);
        logFile.print("|");
        logFile.print(RPM);
        logFile.print("|");
        logFile.print(UV_index);
        logFile.print("|");
        logFile.print(luminosity);
        logFile.flush();
      }

      lastMinute = now.minute();
      digitalWrite(LED, LOW);
    } else {
      if (digitalRead(REED)) {
        if (!pulseSwitched) {
          pulses++;
          pulseSwitched = true;
        }
      } else {
        pulseSwitched = false;
      }
    }
  }
}
