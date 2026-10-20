void executarTarefasPeriodicas() {
  static unsigned long lastWiFiReconnectAttempt = 0;
  static unsigned long lastNtpRetry = 0;
  static unsigned long lastSolarRetry = 0;
  static unsigned long lastLedUpdate = 0;
  static int lastDaySolarSync = -1;
  static bool ledsDesligadosSemHorario = false;
  unsigned long currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillis - lastWiFiReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
      lastWiFiReconnectAttempt = currentMillis;
      conectarWiFi();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }

  if (!horarioSincronizado()) {
    if (WiFi.status() == WL_CONNECTED && currentMillis - lastNtpRetry >= WIFI_RECONNECT_INTERVAL_MS) {
      lastNtpRetry = currentMillis;
      sincronizarHorario();
    }

    if (!ledsDesligadosSemHorario) {
      desligarLedsAquario();
      ledsDesligadosSemHorario = true;
    }
    return;
  }

  ledsDesligadosSemHorario = false;

  int currentMinute = minute(timeClient.getEpochTime());
  int currentHour = hour(timeClient.getEpochTime());
  int currentDay = day(timeClient.getEpochTime());

  if (currentHour == SUN_SYNC_HOUR && currentMinute == SUN_SYNC_MINUTE && currentDay != lastDaySolarSync) {
    if (atualizarHorariosSol()) {
      lastDaySolarSync = currentDay;
    }
  }

  if (!horariosSolaresCarregados && currentMillis - lastSolarRetry >= WIFI_RECONNECT_INTERVAL_MS) {
    lastSolarRetry = currentMillis;
    atualizarHorariosSol();
  }

  if (currentMillis - lastLedUpdate >= LED_UPDATE_INTERVAL) {
    controlarLeds();
    lastLedUpdate = currentMillis;
  }
}
