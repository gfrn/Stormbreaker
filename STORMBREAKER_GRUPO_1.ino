/* Código Stormbreaker - Grupo 1
    Integrantes:
    Andrielly Garcia
    Gabriel Azevedo
    Guilherme Freitas
    Murilo Souza
    Patrick Oliveira

    Última revisão: 29/08/2019
    Versão: 0.1.0.0

    Bibliotecas necessárias implícitas:
    Adafruit Unified Sensor

    Utilização de memória (UNO): 58%
*/

#include <DHT.h>
#include <SD.h>
#include <DS1307.h>

const int LDR = A0;
const int DHTP = A1;
const int UV = A2;
const int REED = 2;
const int LED = 3;
const int SDCS = 10;
const int DHTTYPE = DHT11;

DS1307 rtc(A4, A5);

DHT dht(DHTP, DHTTYPE);

float UV_index = 0;
float humidity = 0;
float temp = 0;
float RPM = 0;
float luminosity = 0;

int interval = 1;

Sd2Card SDcard;
SdVolume volume;

int pulses = 0;
bool pulseSwitched = 0; //True enquanto o sensor reed está HIGH depois de incrementar o número de pulsos para evitar falsos positivos de rotação. Reseta em LOW.

void setup() {
  pinMode(LDR, INPUT);
  pinMode(DHTP, INPUT);
  pinMode(UV, INPUT);
  pinMode(REED, INPUT);
  pinMode(LED, OUTPUT);

  Serial.begin(9600);

  rtc.halt(false); //Ativa RTC

  dht.begin();

  rtc.setSQWRate(SQW_RATE_1); //Delimita rate de polling/interface entre o sensor e Arduino
  rtc.enableSQW(true);

  if (!SD.begin(SDCS))
  {
    Serial.println("Erro inicializando o cartao!");

  }
}

String parseDate(String date)
{
  //Retira valores em inteiro da string enviada pelo programa de computador para o Arduino quando em modo configuração
  int seconds = date.substring(15, 16).toInt();
  int minutes = date.substring(12, 13).toInt();
  int hours = date.substring(9, 10).toInt();
  int weekDay = date.substring(18, 18).toInt();
  int days = date.substring(0, 1).toInt();
  int months = date.substring(3, 4).toInt();
  int years  = date.substring(6, 7).toInt();
  
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(days, months, years);
}

String readDate()
{
  //Concatena valores de data e hora
  String date = rtc.getDateStr(FORMAT_SHORT);
  String hour = rtc.getTimeStr(FORMAT_SHORT);
  return date + " " + hour;
}

int getUV()
{
  //Calcula índice UV baseado em leitura da porta analógica, onde Vout max. do sensor é 1.2V
  double UVRead = analogRead(UV);

  return UVRead / 1024 * 3.3;
}

void loop() {
  if (Serial.available())
  {
    String valueSerial = Serial.readString();
    char interpret = valueSerial.charAt(0);

    switch (interpret)
    {
      case 'd':
        //Computador envia sinal pedindo mudança de horário salvo do RTC
        parseDate(Serial.readString().substring(2));
        break;
      case 'l':
        //Computador pede a transferência das informações no log salvo no cartão SD
        File logFile = SD.open("log.txt", FILE_READ);

        if (!logFile)
        {
          Serial.println("Erro na abertura do arquivo!");
        }
        else
        {
          while (logFile.available())
          {
            Serial.println(logFile.readString());
          }
        }
        break;
      case 'i':
        //Muda intervalo de leitura de sensores
        interval = Serial.readString().substring(2).toInt();
        break;
    }
  }
  else
  {
    digitalWrite(LED, HIGH);

    //Realiza leitura de sensores e tradução de valores
    UV_index = getUV();
    pulses = 0;
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    luminosity = analogRead(LDR) / 1024 * 100;
    String date = readDate();

    if (rtc.getTime().min % interval == 0 || rtc.getTime().min == 0)
    {
      File logFile = SD.open("log.txt", FILE_WRITE);

      //LED mostra se o cartão SD está em estado de leitura/escrita para impedir que o usuário desligue o Arduino e corrompa o arquivo
      digitalWrite(LED, HIGH);
      RPM = pulses;
      Serial.println(date + "|" + temp + "|" + humidity + "|" + RPM + "|" + UV_index + "|" + luminosity);
      pulses = 0;
      if (!logFile)
      {
        Serial.println("Erro na abertura do arquivo!");
      }
      else
      {
        logFile.println(date + "|" + temp + "|" + humidity + "|" + RPM + "|" + UV_index + "|" + luminosity);
        logFile.close();
      }
      digitalWrite(LED, LOW);
      delay(1000);
    }
    else
    {
      if (digitalRead(REED))
      {
        if (!pulseSwitched)
        {
          pulses++;
          pulseSwitched = true;
        }
      }
      else
      {
        pulseSwitched = false;
      }
    }
  }
}
