void configurarLeds() {
  pinMode(LED_RELAY_PIN, OUTPUT);
  digitalWrite(LED_RELAY_PIN, LOW);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_PWM_VALUE);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

float suavizarTransicao(float progresso) {
  progresso = constrain(progresso, 0.0, 1.0);
  return progresso * progresso * (3.0 - 2.0 * progresso);
}

int interpolarBrilho(int inicio, int fim, int agora, int brilhoInicio, int brilhoFim) {
  if (fim <= inicio) {
    return brilhoFim;
  }

  float progresso = (float)(agora - inicio) / (float)(fim - inicio);
  float curva = suavizarTransicao(progresso);
  return (int)(brilhoInicio + ((brilhoFim - brilhoInicio) * curva));
}

bool horariosSolaresValidos(int amanhecer, int nascer, int zenite, int porDoSol, int anoitecer) {
  return amanhecer < nascer && nascer < zenite && zenite < porDoSol && porDoSol < anoitecer;
}

void desligarLedsAquario() {
  brilho = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  digitalWrite(LED_RELAY_PIN, LOW);
  FastLED.show();
}

void aplicarCoresCicloSolar(int brilhoBase, int minutoTotal, int nascer, int zenite, int porDoSol) {
  bool periodoManha = minutoTotal <= zenite;
  int distanciaZenite = abs(minutoTotal - zenite);
  int maiorDistancia = max(abs(zenite - nascer), abs(porDoSol - zenite));
  float proximidadeZenite = maiorDistancia > 0 ? 1.0 - ((float)distanciaZenite / (float)maiorDistancia) : 1.0;
  proximidadeZenite = constrain(proximidadeZenite, 0.0, 1.0);

  float fatorTransicao = 1.0 - proximidadeZenite;
  uint8_t saturacaoBranco = (uint8_t)(80 - (45 * proximidadeZenite));
  uint8_t brilhoBranco = (uint8_t)constrain(brilhoBase * (0.85 + (0.15 * proximidadeZenite)), 0, LED_PLANTED_TANK_MAX_BRIGHTNESS);
  uint8_t brilhoVermelho = (uint8_t)constrain(brilhoBase * (0.75 + (0.20 * proximidadeZenite)), 0, LED_PLANTED_TANK_MAX_BRIGHTNESS);
  uint8_t brilhoAzul = (uint8_t)constrain(brilhoBase * (0.70 + (0.22 * proximidadeZenite)), 0, LED_PLANTED_TANK_MAX_BRIGHTNESS);
  uint8_t brilhoAmbar = (uint8_t)constrain(brilhoBase * (0.18 + (0.42 * fatorTransicao)), 0, LED_PLANTED_TANK_MAX_BRIGHTNESS);

  for (int i = 0; i < NUM_LEDS; i++) {
    int coluna = i % LEDS_PER_ROW;
    int fileira = (i / LEDS_PER_ROW) % LED_ROWS;
    float fatorFileira = 0.92 + (0.02 * fileira);
    uint8_t brilhoLed = (uint8_t)constrain(brilhoBase * fatorFileira, 0, LED_PLANTED_TANK_MAX_BRIGHTNESS);

    if (fileira == 0) {
      leds[i].setHSV(35, saturacaoBranco, brilhoBranco);
    } else if (fileira == 1) {
      leds[i].setHSV(0, MAX_PWM_VALUE, brilhoVermelho);
    } else if (fileira == 2) {
      leds[i].setHSV(160, MAX_PWM_VALUE, brilhoAzul);
    } else if (fileira == 3) {
      leds[i].setHSV(40, 120, brilhoLed);
    } else {
      uint8_t hue = periodoManha ? 32 : 18;
      leds[i].setHSV(hue, 210, brilhoAmbar);
    }

    if (coluna < 2 || coluna > LEDS_PER_ROW - 3) {
      leds[i].nscale8(230);
    }
  }
}

void controlarLeds() {
  int hora = hour(timeClient.getEpochTime());
  int minuto = minute(timeClient.getEpochTime());
  int minutoTotal = calcularMinutoDoDia(hora, minuto);

  int minutoTotalAmanhecer = calcularMinutoDoDia(HoraAmanhecer, MinuteAmanhecer);
  int minutoTotalNascer = calcularMinutoDoDia(HourNascerDoSol, MinuteNascerDoSol);
  int minutoTotalZenite = calcularMinutoDoDia(HoraZenite, MinuteZenite);
  int minutoTotalPor = calcularMinutoDoDia(HoraPorDoSol, MinutePorDoSol);
  int minutoTotalAnoitecer = calcularMinutoDoDia(HourAnoitecer, MinuteAnoitecer);

  if (minutoTotalZenite <= minutoTotalNascer || minutoTotalZenite >= minutoTotalPor) {
    minutoTotalZenite = minutoTotalNascer + ((minutoTotalPor - minutoTotalNascer) / 2);
  }

  if (!horariosSolaresValidos(minutoTotalAmanhecer, minutoTotalNascer, minutoTotalZenite, minutoTotalPor, minutoTotalAnoitecer)) {
    desligarLedsAquario();
    return;
  }

  int minutoTotalInicioLuz = max(minutoTotalAmanhecer, minutoTotalNascer - LED_PRE_SUNRISE_MINUTES);
  int minutoTotalFimLuz = min(minutoTotalAnoitecer, minutoTotalPor + LED_POST_SUNSET_MINUTES);

  int horaLigarReguaLed = minutoTotalZenite - LED_RELAY_WINDOW_MINUTES;
  int horaDesligarReguaLed = minutoTotalZenite + LED_RELAY_WINDOW_MINUTES;

  if (minutoTotal < minutoTotalInicioLuz || minutoTotal > minutoTotalFimLuz) {
    desligarLedsAquario();
    return;
  }

  if (minutoTotal >= horaLigarReguaLed && minutoTotal <= horaDesligarReguaLed) {
    digitalWrite(LED_RELAY_PIN, HIGH);
  } else {
    digitalWrite(LED_RELAY_PIN, LOW);
  }

  if (minutoTotal < minutoTotalNascer) {
    brilho = interpolarBrilho(minutoTotalInicioLuz, minutoTotalNascer, minutoTotal, 0, LED_SUNRISE_BRIGHTNESS);
  } else if (minutoTotal < minutoTotalZenite) {
    brilho = interpolarBrilho(minutoTotalNascer, minutoTotalZenite, minutoTotal, LED_SUNRISE_BRIGHTNESS, LED_PLANTED_TANK_MAX_BRIGHTNESS);
  } else if (minutoTotal < minutoTotalPor) {
    brilho = interpolarBrilho(minutoTotalZenite, minutoTotalPor, minutoTotal, LED_PLANTED_TANK_MAX_BRIGHTNESS, LED_SUNSET_BRIGHTNESS);
  } else {
    brilho = interpolarBrilho(minutoTotalPor, minutoTotalFimLuz, minutoTotal, LED_SUNSET_BRIGHTNESS, 0);
  }

  brilho = constrain(brilho, 0, MAX_PWM_VALUE);
  aplicarCoresCicloSolar(brilho, minutoTotal, minutoTotalNascer, minutoTotalZenite, minutoTotalPor);
  FastLED.show();
}
