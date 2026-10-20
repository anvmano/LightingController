# LightingController

Firmware Arduino para controlar a iluminacao de um aquario plantado. Sincroniza o horario por NTP, consulta os horarios solares do dia no Firebase Realtime Database e aplica uma curva de brilho e cores em LEDs enderecaveis. Um rele aciona a regua LED branca ao redor do zenite.

## Hardware identificado

A documentacao do projeto identifica ESP32/Wemos D1 R32, com alvo Arduino `esp32:esp32:d1_uno32`. O codigo usa:

| Componente | Configuracao em `config.h` |
| --- | --- |
| LEDs enderecaveis NeoPixel, controlados por FastLED | GPIO14, 125 LEDs, layout logico de 5 fileiras de 25 |
| Rele da regua LED branca | GPIO27, acionado em HIGH e desligado em LOW |

Este firmware nao le sensores. Os sensores citados em `docs/SPLIT_PROJECTS.md` pertencem ao projeto separado `SensorsController`. Nao ha esquema eletrico neste repositorio; os pinos acima sao os definidos no codigo, sem inferir outras ligacoes ou modelos de componentes.

## Bibliotecas

- Core ESP32: `WiFi.h`, `HTTPClient.h`, `WiFiClientSecure.h` e `WiFiUdp.h`.
- NTPClient (`NTPClient.h`).
- Time (`TimeLib.h`).
- ArduinoJson 7 (`ArduinoJson.h`; o codigo usa `JsonDocument`).
- FastLED (`FastLED.h`).

## Configuracao local

1. Mantenha o sketch na pasta `LightingController`, com `LightingController.ino` e as demais abas na mesma pasta.
2. Copie `secrets.example.h` para `secrets.h`. No PowerShell:

   ```powershell
   Copy-Item secrets.example.h secrets.h
   ```

3. Edite somente o arquivo local `secrets.h`: preencha `ssid`, `password`, `FIREBASE_HOST` e `FIREBASE_AUTH`. O host deve ser a URL HTTPS do seu Firebase Realtime Database; a credencial deve permitir a leitura dos dados solares. Preserve os nomes, tipos e declaracoes do exemplo.
4. Disponibilize os dados solares do dia em `/historico/NascePorDoSol/DD-MM-AAAA`. O firmware espera um objeto com filhos contendo `HoraAmanhecer`, `MinuteAmanhecer`, `HourNascerDoSol`, `MinuteNascerDoSol`, `HoraPorDoSol`, `MinutePorDoSol`, `HourAnoitecer` e `MinuteAnoitecer`. `HoraZenite` e `MinuteZenite` podem ser omitidos: nesse caso, o firmware usa o ponto medio entre nascer e por do sol. A ordem exigida e amanhecer < nascer < zenite < por do sol < anoitecer. Este firmware apenas consome esses dados; nao os gera.

`secrets.h` e ignorado pelo Git. Publique apenas `secrets.example.h`, mantendo os placeholders. Credenciais reais nao devem ser colocadas em codigo, comentarios, documentacao ou exemplos versionados. O `.gitignore` tambem exclui arquivos de credenciais, chaves privadas, backups, logs, compilacao e configuracoes pessoais comuns.

## Compilacao

Na Arduino IDE, instale o pacote de placas ESP32 e as bibliotecas acima, selecione a placa WEMOS D1 R32 e abra `LightingController.ino`. Crie `secrets.h` antes de verificar/compilar.

Com Arduino CLI e o indice de placas ESP32 ja configurado, execute na pasta do sketch:

```powershell
arduino-cli core install esp32:esp32@2.0.14
arduino-cli lib install "NTPClient@3.2.1" "Time@1.6.1" "ArduinoJson@7.4.3" "FastLED@3.10.7"
arduino-cli compile --warnings all --fqbn esp32:esp32:d1_uno32 .
```

Essas sao as versoes utilizadas na verificacao de publicacao. A compilacao nao exige conectar uma placa; o envio e o teste fisico exigem o hardware correspondente e a selecao da porta na IDE/CLI.

As constantes de pinagem, brilho, intervalos e horario UTC-3 permanecem em `config.h`. A estrutura, a logica e os valores do firmware foram preservados na copia publicada.

## Documentacao

Os arquivos em `docs/` descrevem arquitetura, dependencias, regras de iluminacao e a divisao em dois projetos. As referencias a `secrets.h` nesses documentos se referem ao arquivo criado localmente a partir do exemplo.
