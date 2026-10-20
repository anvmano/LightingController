String obterDataFormatada() {
  char buf[11];
  sprintf(buf, "%02d-%02d-%04d", day(timeClient.getEpochTime()), month(timeClient.getEpochTime()), year(timeClient.getEpochTime()));
  return String(buf);
}

int calcularMinutoDoDia(int hora, int minuto) {
  return (hora * 60) + minuto;
}

bool horarioSincronizado() {
  return year(timeClient.getEpochTime()) >= MIN_VALID_YEAR;
}

bool sincronizarHorario() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Horario nao sincronizado: Wi-Fi desconectado.");
    return false;
  }

  bool sincronizou = timeClient.forceUpdate();
  if (!sincronizou || !horarioSincronizado()) {
    Serial.println("Horario nao sincronizado: falha ao consultar NTP.");
    return false;
  }

  Serial.print("Horario sincronizado: ");
  Serial.println(obterDataFormatada());
  return true;
}
