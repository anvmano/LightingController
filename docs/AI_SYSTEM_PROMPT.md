# AI_SYSTEM_PROMPT

Voce esta trabalhando no projeto `LightingController`, um firmware Arduino para ESP32/Wemos D1 R32.

## Objetivo do sistema

Controlar a iluminacao de um aquario plantado usando uma curva diaria baseada em horarios solares carregados do Firebase Realtime Database. O firmware tambem controla um rele para uma regua LED branca ao redor do zenite.

## Escopo atual

Este projeto contem apenas iluminacao:

- Wi-Fi.
- NTP.
- Leitura REST de horarios solares no Firebase.
- Fita NeoPixel/FastLED.
- Rele da regua LED branca.

Ele nao contem sensores, gravacao historica, MQ135, BME680, DS18B20, TDS, pH, turbidez, fan, servidor web local, interface grafica ou banco SQL. Esses itens pertencem ao projeto separado `SensorsController` ou ao firmware antigo.

## Tecnologias

- Arduino/C++.
- ESP32 alvo compativel com `esp32:esp32:d1_uno32`.
- Wi-Fi via `WiFi.h`.
- HTTPS/REST via `HTTPClient.h` e `WiFiClientSecure.h`.
- NTP via `NTPClient`.
- Datas via `TimeLib`.
- JSON via `ArduinoJson`.
- LEDs via `FastLED`.

## Arquivos importantes

- `LightingController.ino`: globais, `setup()` e `loop()`.
- `config.h`: constantes de hardware, brilho e agenda.
- `secrets.h`: credenciais sensiveis. Nao expor.
- `01_Utils.ino`: horario, data e NTP.
- `02_Connectivity.ino`: Wi-Fi.
- `03_WeatherAndSun.ino`: leitura e parsing dos horarios solares.
- `04_Actuators.ino`: LEDs, curva de brilho, cores e rele.
- `05_Tasks.ino`: tarefas periodicas.

## Fluxos principais

Boot:

1. Serial.
2. Configuracao inicial dos LEDs e rele desligados.
3. Tentativa de conexao Wi-Fi com timeout.
4. Inicio do NTP.
5. Sincronizacao de horario.
6. Leitura dos horarios solares.
7. Controle inicial dos LEDs ou desligamento por falha.

Loop:

1. Tenta reconectar Wi-Fi em intervalo configurado.
2. Atualiza NTP quando Wi-Fi esta conectado.
3. Se horario invalido, tenta novo NTP, desliga LEDs/rele e retorna antes de Firebase/LEDs.
4. Atualiza horarios solares uma vez ao dia em `SUN_SYNC_HOUR:SUN_SYNC_MINUTE`.
5. Faz retry da leitura solar enquanto nenhum horario valido foi carregado.
6. Atualiza LEDs a cada `LED_UPDATE_INTERVAL`.

## Regras criticas

- Nao controlar LEDs com horario NTP invalido.
- Desligar LEDs e rele quando horario invalido ou horarios solares invalidos.
- Manter leitura solar em `/historico/NascePorDoSol/DD-MM-AAAA`.
- Validar ordem: amanhecer < nascer < zenite < por do sol < anoitecer.
- Manter fallback de zenite quando zenite vier ausente/invalido.
- Lembrar que o fallback de zenite tambem e aplicado localmente em `controlarLeds()` quando o zenite global esta fora da janela nascer/por do sol.
- Nao publicar credenciais de `secrets.h`.
- Alteracoes em `04_Actuators.ino` afetam iluminacao fisica do aquario.
- Alteracoes em `config.h` podem exigir revisao de hardware.

## Nunca altere sem necessidade

- Valores e estrutura de `secrets.h`.
- Pinos `LED_PIN` e `LED_RELAY_PIN`.
- Caminho Firebase de horarios solares.
- Validacao de horarios solares.
- Desligamento de LEDs em falha de horario ou dados solares.
- Janela do rele ao redor do zenite sem considerar fotoperiodo real.

## Checklist antes de modificar codigo

- A mudanca afeta horario, Firebase, Wi-Fi ou atuador fisico?
- A mudanca depende de dado produzido pelo `SensorsController`?
- O caminho REST continua correto?
- A curva de LEDs continua protegida contra horarios fora de ordem?
- O rele continua desligando fora da janela configurada?
- A alteracao exige teste fisico da fita ou rele?

## Checklist antes de finalizar alteracoes

- Compilar com `arduino-cli compile --warnings all --fqbn esp32:esp32:d1_uno32`, quando a CLI estiver disponivel.
- Conferir que `setup()` e `loop()` continuam simples.
- Confirmar que nenhuma credencial foi exposta.
- Confirmar que falhas de Wi-Fi/NTP/Firebase deixam LEDs em estado seguro.
- Confirmar que os campos solares esperados continuam documentados.
