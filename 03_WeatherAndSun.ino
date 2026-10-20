bool atualizarHorariosSol() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Horarios solares nao atualizados: Wi-Fi desconectado.");
    return false;
  }

  String dataAtual = obterDataFormatada();
  String basePath = "/historico/NascePorDoSol/" + dataAtual;
  String url = montarUrlFirebase(basePath);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(FIREBASE_HTTP_TIMEOUT_MS);

  if (!http.begin(client, url)) {
    Serial.println("Erro ao iniciar HTTP para horarios solares.");
    return false;
  }

  int statusCode = http.GET();
  if (statusCode == HTTP_CODE_OK) {
    String stringfirebase = http.getString();
    http.end();
    return processarJsonHorariosSol(stringfirebase);
  }

  Serial.print("Erro HTTP ao ler horarios solares: ");
  Serial.println(statusCode);
  http.end();
  return false;
}

String montarUrlFirebase(const String& path) {
  String baseUrl = FIREBASE_HOST;
  if (!baseUrl.endsWith("/")) {
    baseUrl += "/";
  }

  String cleanPath = path;
  if (cleanPath.startsWith("/")) {
    cleanPath.remove(0, 1);
  }

  return baseUrl + cleanPath + ".json?auth=" + FIREBASE_AUTH;
}

bool processarJsonHorariosSol(const String& jsonString) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Erro ao fazer o parsing do JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  bool encontrouHorarioValido = false;
  for (JsonPair pair : doc.as<JsonObject>()) {
    if (!pair.value().is<JsonObject>()) {
      continue;
    }

    JsonObject nestedObj = pair.value().as<JsonObject>();
    int horaAmanhecer = nestedObj["HoraAmanhecer"] | -1;
    int minuteAmanhecer = nestedObj["MinuteAmanhecer"] | -1;
    int hourNascerDoSol = nestedObj["HourNascerDoSol"] | -1;
    int minuteNascerDoSol = nestedObj["MinuteNascerDoSol"] | -1;
    int horaPorDoSol = nestedObj["HoraPorDoSol"] | -1;
    int minutePorDoSol = nestedObj["MinutePorDoSol"] | -1;
    int hourAnoitecer = nestedObj["HourAnoitecer"] | -1;
    int minuteAnoitecer = nestedObj["MinuteAnoitecer"] | -1;
    int horaZenite = nestedObj["HoraZenite"] | -1;
    int minuteZenite = nestedObj["MinuteZenite"] | -1;

    int amanhecer = calcularMinutoDoDia(horaAmanhecer, minuteAmanhecer);
    int nascer = calcularMinutoDoDia(hourNascerDoSol, minuteNascerDoSol);
    int porDoSol = calcularMinutoDoDia(horaPorDoSol, minutePorDoSol);
    int anoitecer = calcularMinutoDoDia(hourAnoitecer, minuteAnoitecer);
    int zenite = calcularMinutoDoDia(horaZenite, minuteZenite);

    if (horaZenite < 0 || minuteZenite < 0) {
      zenite = nascer + ((porDoSol - nascer) / 2);
      horaZenite = zenite / 60;
      minuteZenite = zenite % 60;
    }

    if (!horariosSolaresValidos(amanhecer, nascer, zenite, porDoSol, anoitecer)) {
      Serial.println("Horarios solares invalidos no Firebase. Mantendo ultimos valores validos.");
      continue;
    }

    HoraAmanhecer = horaAmanhecer;
    MinuteAmanhecer = minuteAmanhecer;
    HourNascerDoSol = hourNascerDoSol;
    MinuteNascerDoSol = minuteNascerDoSol;
    HoraZenite = horaZenite;
    MinuteZenite = minuteZenite;
    HoraPorDoSol = horaPorDoSol;
    MinutePorDoSol = minutePorDoSol;
    HourAnoitecer = hourAnoitecer;
    MinuteAnoitecer = minuteAnoitecer;
    encontrouHorarioValido = true;
  }

  horariosSolaresCarregados = encontrouHorarioValido || horariosSolaresCarregados;
  return encontrouHorarioValido;
}
