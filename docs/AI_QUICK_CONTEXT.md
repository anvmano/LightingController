# AI_QUICK_CONTEXT

## O que e o sistema

`LightingController` e um firmware Arduino para ESP32/Wemos D1 R32 responsavel exclusivamente pela iluminacao de um aquario plantado. Ele conecta ao Wi-Fi, sincroniza horario via NTP, le horarios solares no Firebase Realtime Database por REST/HTTPS e controla uma fita NeoPixel/FastLED e um rele de regua LED branca.

Caminho atual do projeto:

```text
LightingController/
```

Este projeto nasceu da divisao do firmware antigo `WebServer` em dois sketches independentes:

- `SensorsController`: sensores e gravacao historica.
- `LightingController`: horarios solares, LEDs e rele.

## Tecnologias

- Arduino/C++ em abas `.ino`.
- Placa alvo esperada: ESP32/Wemos D1 R32, compativel com `esp32:esp32:d1_uno32`.
- Wi-Fi via core ESP32.
- NTP via `NTPClient`, `WiFiUdp` e `TimeLib`.
- Firebase Realtime Database via REST usando `HTTPClient` e `WiFiClientSecure`.
- JSON via `ArduinoJson`.
- LEDs enderecaveis via `FastLED`.

## Arquitetura Resumida

Arquivos ativos:

- `LightingController.ino`: includes, globais, `setup()` e `loop()`.
- `config.h`: pinos, intervalos, limites de brilho e agenda.
- `secrets.h`: credenciais Wi-Fi e Firebase. Nao publicar.
- `01_Utils.ino`: data, horario, NTP e calculo de minutos do dia.
- `02_Connectivity.ino`: conexao Wi-Fi.
- `03_WeatherAndSun.ino`: leitura REST dos horarios solares no Firebase.
- `04_Actuators.ino`: curva de LEDs, cores, rele e desligamento.
- `05_Tasks.ino`: tarefas periodicas no loop.

## Fluxo Principal

No `setup()`:

1. Inicia Serial.
2. Configura LEDs e rele desligados.
3. Tenta conectar Wi-Fi com timeout.
4. Inicializa NTP com offset UTC-3.
5. Sincroniza horario.
6. Le horarios solares do Firebase.
7. Controla LEDs se houver horario e dados solares validos; caso contrario, desliga tudo.

No `loop()`:

- `executarTarefasPeriodicas()` tenta reconectar Wi-Fi em intervalo configurado, atualiza NTP quando conectado, tenta nova sincronizacao se o horario ainda for invalido, atualiza horarios solares diariamente em `SUN_SYNC_HOUR:SUN_SYNC_MINUTE`, faz retry da leitura solar enquanto nenhum horario valido foi carregado e atualiza LEDs a cada `LED_UPDATE_INTERVAL`.
- Se o horario NTP estiver invalido, a rotina desliga LEDs/rele e retorna antes de atualizar Firebase ou LEDs.
- Se o Wi-Fi cair depois de horario e horarios solares validos ja existirem, o firmware tenta reconectar, mas a curva pode continuar usando o ultimo horario NTP valido e os ultimos dados solares validos.

## Firebase

Leitura:

- `/historico/NascePorDoSol/DD-MM-AAAA`

O codigo monta a URL:

```text
FIREBASE_HOST/historico/NascePorDoSol/DD-MM-AAAA.json?auth=FIREBASE_AUTH
```

Campos esperados dentro de cada objeto filho:

- `HoraAmanhecer`
- `MinuteAmanhecer`
- `HourNascerDoSol`
- `MinuteNascerDoSol`
- `HoraZenite`
- `MinuteZenite`
- `HoraPorDoSol`
- `MinutePorDoSol`
- `HourAnoitecer`
- `MinuteAnoitecer`

Se `HoraZenite` ou `MinuteZenite` estiver ausente/invalido, o firmware calcula fallback como ponto medio entre nascer do sol e por do sol.

O JSON esperado no caminho do dia e um objeto com filhos dinamicos; cada filho pode conter um conjunto de horarios solares. O firmware percorre todos os filhos e aplica o ultimo filho valido encontrado. Filhos invalidos sao ignorados e nao sobrescrevem horarios validos ja existentes.

## Pinos e Hardware

- NeoPixel/FastLED: GPIO14.
- Quantidade de LEDs: 125.
- Layout logico: 5 fileiras de 25 LEDs.
- Rele da regua LED branca: GPIO27.
- Wi-Fi e Firebase dependem de `secrets.h`.

## Regras Criticas

- Sem horario NTP valido (`year >= MIN_VALID_YEAR`), os LEDs sao desligados e a rotina retorna.
- Horarios solares precisam obedecer: amanhecer < nascer < zenite < por do sol < anoitecer.
- Horarios solares invalidos nao sobrescrevem os ultimos valores validos.
- Depois que pelo menos um horario solar valido foi carregado, falhas posteriores de leitura Firebase nao apagam os horarios atuais.
- O rele branco liga em uma janela centrada no zenite.
- A fita so acende entre `nascer - LED_PRE_SUNRISE_MINUTES` e `por do sol + LED_POST_SUNSET_MINUTES`, respeitando limites de amanhecer/anoitecer.
- Fora da janela de luz, ou quando a ordem solar aplicada e invalida, `desligarLedsAquario()` zera o brilho, apaga a fita e coloca o rele em `LOW`.
- `secrets.h` contem credenciais reais e nao deve ser exposto.

## Resumo para IA

Antes de modificar, identifique se a alteracao afeta conectividade, horario, leitura solar ou atuadores. Preserve o caminho Firebase de horarios solares, a validacao de ordem dos horarios e o comportamento de desligar LEDs quando horario/dados solares nao sao confiaveis. Para validar, compile com `arduino-cli compile --warnings all --fqbn esp32:esp32:d1_uno32`.
