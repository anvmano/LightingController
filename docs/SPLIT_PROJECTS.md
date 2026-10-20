# Divisao em dois firmwares

O projeto original `WebServer` foi separado em dois sketches Arduino independentes para reduzir acoplamento e deixar cada firmware com uma responsabilidade clara.

## SensorsController

Pasta esperada: `SensorsController/`

Responsavel por leitura, processamento e historico:

- Wi-Fi;
- NTP;
- Firebase Realtime Database;
- sensores ambientais;
- sensores de aquario;
- gravacao em `/historico/AirQuality/...`;
- gravacao em `/historico/Aquario/...`.

Esse projeto e o lugar correto para regras de BME680, MQ135, DS18B20, TDS, pH, turbidez, calibracao de sensores e contrato de escrita historica.

## LightingController

Pasta atual:

```text
LightingController/
```

Responsavel por iluminacao:

- Wi-Fi;
- NTP;
- leitura de horarios solares em `/historico/NascePorDoSol/DD-MM-AAAA`;
- parsing de filhos dinamicos no objeto Firebase do dia;
- fita FastLED/NeoPixel;
- rele da regua branca;
- curva diaria por amanhecer, nascer do sol, zenite, por do sol e anoitecer.

Arquivos ativos:

- `LightingController.ino`;
- `config.h`;
- `secrets.h`;
- `01_Utils.ino`;
- `02_Connectivity.ino`;
- `03_WeatherAndSun.ino`;
- `04_Actuators.ino`;
- `05_Tasks.ino`.

## O que saiu do LightingController

O `LightingController` nao contem mais:

- BME680;
- MQ135;
- DS18B20;
- TDS;
- pH;
- turbidez;
- gravacao de historico;
- funcoes de Firebase `pushJSON`;
- fan.

## Contrato entre projetos

O `LightingController` consome dados solares no Firebase:

```text
/historico/NascePorDoSol/DD-MM-AAAA
```

Campos esperados:

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

Quem produzir esses dados deve manter a estrutura e a data no formato `DD-MM-AAAA`.

No caminho do dia, o `LightingController` espera um objeto com um ou mais filhos dinamicos. Cada filho deve conter os campos solares acima. O firmware percorre os filhos, ignora entradas invalidas e aplica o ultimo conjunto valido encontrado.

## Observacoes

- `secrets.h` existe no projeto atual porque a IDE/CLI do Arduino costuma trabalhar melhor com credenciais dentro da pasta do sketch.
- As credenciais nao devem ser documentadas nem publicadas.
- Alteracoes em sensores devem ser feitas no `SensorsController`.
- Alteracoes em curva, LEDs, rele e leitura solar devem ser feitas no `LightingController`.
