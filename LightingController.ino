#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "config.h"
#include "secrets.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int HoraPorDoSol = 0;
int MinutePorDoSol = 0;
int HourNascerDoSol = 0;
int MinuteNascerDoSol = 0;
int HoraAmanhecer = 0;
int MinuteAmanhecer = 0;
int HoraZenite = 0;
int MinuteZenite = 0;
int HourAnoitecer = 0;
int MinuteAnoitecer = 0;

CRGB leds[NUM_LEDS];
int brilho = 0;
bool horariosSolaresCarregados = false;

void setup() {
  Serial.begin(9600);

  configurarLeds();
  conectarWiFi();

  timeClient.begin();
  timeClient.setTimeOffset(NTP_TIME_OFFSET_SECONDS);

  if (sincronizarHorario() && atualizarHorariosSol()) {
    controlarLeds();
  } else {
    desligarLedsAquario();
  }
}

void loop() {
  executarTarefasPeriodicas();
}
