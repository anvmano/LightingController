# ARCHITECTURE_AI

## Visao Geral

`LightingController` e o firmware de iluminacao extraido do projeto Arduino antigo `WebServer`. Ele roda em ESP32/Wemos D1 R32 e tem uma responsabilidade clara: buscar no Firebase os horarios solares do dia e aplicar uma curva de iluminacao em uma fita NeoPixel, alem de acionar uma regua LED branca por rele perto do zenite.

O codigo atual contem:

- conexao Wi-Fi com timeout;
- sincronizacao de horario por NTP;
- leitura HTTPS/REST do Firebase Realtime Database;
- parsing de JSON com `ArduinoJson`;
- validacao de horarios solares;
- fallback de zenite quando ausente;
- controle de fita FastLED/NeoPixel;
- curva de brilho com transicao suave;
- cores diferentes por fileira de LED;
- rele ligado em janela ao redor do zenite;
- tarefas periodicas para reconexao, NTP, atualizacao solar e LEDs.

Nao ha sensores, gravacao no Firebase, interface local, servidor HTTP, testes automatizados ou fan neste projeto.

## Estrutura Fisica

Caminho atual do sketch:

```text
LightingController/
```

```text
LightingController/
  LightingController.ino
  config.h
  secrets.h
  01_Utils.ino
  02_Connectivity.ino
  03_WeatherAndSun.ino
  04_Actuators.ino
  05_Tasks.ino
```

Responsabilidades:

- `LightingController.ino`: arquivo principal do sketch. Declara includes, objetos globais, variaveis solares, `setup()` e `loop()`.
- `config.h`: constantes de Wi-Fi, Firebase HTTP, NTP, LEDs, rele e agenda solar.
- `secrets.h`: SSID, senha, host Firebase e auth. Nao publicar.
- `01_Utils.ino`: formatacao de data, calculo de minuto do dia, validacao e sincronizacao NTP.
- `02_Connectivity.ino`: conexao Wi-Fi com hostname `LightingController`.
- `03_WeatherAndSun.ino`: montagem da URL Firebase, GET HTTPS e processamento do JSON solar.
- `04_Actuators.ino`: inicializacao de LEDs/rele, curva de brilho, validacao solar e aplicacao de cores.
- `05_Tasks.ino`: agenda periodica executada pelo `loop()`.

## Fluxo de Execucao

### Inicializacao (`setup()`)

1. Inicia Serial em 9600.
2. Chama `configurarLeds()`, deixando fita e rele desligados.
3. Chama `conectarWiFi()`; a funcao pode retornar `false` por timeout, mas o `setup()` segue para a etapa de NTP.
4. Inicia `timeClient`.
5. Aplica offset `NTP_TIME_OFFSET_SECONDS`.
6. Chama `sincronizarHorario()`.
7. Se horario sincronizou, chama `atualizarHorariosSol()`.
8. Se a sincronizacao NTP e a leitura solar forem validas, chama `controlarLeds()`.
9. Se NTP ou leitura solar falharem, chama `desligarLedsAquario()`.

### Execucao Continua (`loop()`)

`loop()` chama somente `executarTarefasPeriodicas()`.

Em `executarTarefasPeriodicas()`:

- reconecta Wi-Fi a cada `WIFI_RECONNECT_INTERVAL_MS` quando desconectado;
- chama `timeClient.update()` quando conectado;
- se horario estiver invalido, tenta `sincronizarHorario()` em intervalo de reconexao, desliga LEDs uma vez e retorna sem atualizar Firebase ou LEDs;
- se Wi-Fi estiver desconectado mas o horario ainda for considerado valido, a rotina pode continuar controlando LEDs com os ultimos dados solares validos;
- atualiza horarios solares diariamente em `SUN_SYNC_HOUR:SUN_SYNC_MINUTE`;
- enquanto nenhum horario solar valido foi carregado, tenta nova leitura em intervalo de reconexao;
- chama `controlarLeds()` a cada `LED_UPDATE_INTERVAL`.

## Componentes

### Wi-Fi

- Arquivo: `02_Connectivity.ino`.
- Funcao: `conectarWiFi()`.
- Usa `ssid` e `password` de `secrets.h`.
- Timeout: `WIFI_CONNECT_TIMEOUT_MS`.
- Hostname: `LightingController`.

### NTP e Horario

- Arquivo: `01_Utils.ino`.
- Objeto: `NTPClient timeClient`.
- Servidor: `pool.ntp.org`.
- Offset: `NTP_TIME_OFFSET_SECONDS`, atualmente UTC-3.
- Horario valido quando `year(timeClient.getEpochTime()) >= MIN_VALID_YEAR`.
- Data formatada em `DD-MM-AAAA`, usada no caminho Firebase.

### Firebase REST

- Arquivo: `03_WeatherAndSun.ino`.
- Funcao principal: `atualizarHorariosSol()`.
- Cliente TLS: `WiFiClientSecure` com `setInsecure()`.
- HTTP: `HTTPClient`.
- Timeout: `FIREBASE_HTTP_TIMEOUT_MS`.
- URL gerada por `montarUrlFirebase(path)`.
- Caminho lido: `/historico/NascePorDoSol/DD-MM-AAAA`.

### Parsing Solar

- Arquivo: `03_WeatherAndSun.ino`.
- Funcao: `processarJsonHorariosSol(const String& jsonString)`.
- Espera um objeto Firebase contendo filhos dinamicos, cada filho com os campos solares.
- Ignora filhos que nao sejam objetos.
- Valida cada filho e aplica o ultimo horario valido encontrado na iteracao.
- Mantem horarios anteriores quando o novo JSON nao tem horario valido.
- Marca `horariosSolaresCarregados` como verdadeiro depois que pelo menos uma leitura valida acontece; falhas posteriores nao zeram essa flag.

### LEDs

- Arquivo: `04_Actuators.ino`.
- Biblioteca: `FastLED`.
- Pino: `LED_PIN`.
- Quantidade: `NUM_LEDS`.
- Layout logico: `LED_ROWS` x `LEDS_PER_ROW`.
- Brilho maximo da curva: `LED_PLANTED_TANK_MAX_BRIGHTNESS`.
- Atualizacao periodica: `LED_UPDATE_INTERVAL`.
- Fora da janela de luz ou com horarios solares aplicados fora de ordem, a fita e apagada e o rele e desligado.
- A cor e calculada por fileira: branco, vermelho, azul, neutro e ambar, com reducao nas colunas das extremidades.

### Rele da Regua Branca

- Arquivo: `04_Actuators.ino`.
- Pino: `LED_RELAY_PIN`.
- Liga quando o minuto atual fica entre:

```text
zenite - LED_RELAY_WINDOW_MINUTES
zenite + LED_RELAY_WINDOW_MINUTES
```

Fora da janela ou em falha, o rele fica `LOW`.

## Fluxo de Dados

```text
NTP
  -> data DD-MM-AAAA
  -> URL Firebase
  -> JSON de horarios solares com filhos dinamicos
  -> variaveis globais de amanhecer/nascer/zenite/por do sol/anoitecer
  -> controlarLeds()
  -> FastLED + rele
```

## Firebase

### Leitura NascePorDoSol

Caminho:

```text
/historico/NascePorDoSol/DD-MM-AAAA
```

Campos esperados:

- `HoraAmanhecer`
- `MinuteAmanhecer`
- `HourNascerDoSol`
- `MinuteNascerDoSol`
- `HoraPorDoSol`
- `MinutePorDoSol`
- `HourAnoitecer`
- `MinuteAnoitecer`
- `HoraZenite`
- `MinuteZenite`

Se zenite nao existir, o codigo calcula:

```text
zenite = nascer + ((porDoSol - nascer) / 2)
```

Se o zenite global aplicado em `controlarLeds()` estiver fora da janela nascer/por do sol, a curva usa novamente o ponto medio para aquele ciclo de controle, sem regravar as variaveis globais.

## Configuracoes

Todas em `config.h`:

- Wi-Fi: timeout e intervalo de reconexao.
- Firebase HTTP: timeout.
- NTP: offset UTC-3 e ano minimo valido.
- LEDs: pino 14, 125 LEDs, 5 fileiras, 25 LEDs por fileira.
- Rele: GPIO27.
- FastLED usa brilho global `MAX_PWM_VALUE`; a curva propria limita o brilho efetivo por `LED_PLANTED_TANK_MAX_BRIGHTNESS`.
- Brilho: nascer/por do sol 55; maximo plantado 230.
- Fotoperiodo: pre-nascer 20 min; pos-por do sol 20 min.
- Rele: janela de 120 min antes e 120 min depois do zenite.
- Sincronizacao solar diaria: 04:50.

## Call Graph

```text
Arduino runtime
  -> setup()
    -> configurarLeds()
    -> conectarWiFi()
    -> timeClient.begin()
    -> timeClient.setTimeOffset()
    -> sincronizarHorario()
      -> timeClient.forceUpdate()
    -> atualizarHorariosSol()
      -> montarUrlFirebase()
      -> HTTPClient.GET()
      -> processarJsonHorariosSol()
        -> horariosSolaresValidos()
    -> controlarLeds()
      -> horariosSolaresValidos()
      -> interpolarBrilho()
        -> suavizarTransicao()
      -> aplicarCoresCicloSolar()
      -> FastLED.show()
  -> loop()
    -> executarTarefasPeriodicas()
      -> conectarWiFi()
      -> timeClient.update()
      -> sincronizarHorario()
      -> desligarLedsAquario()
      -> atualizarHorariosSol()
      -> controlarLeds()
```

## Arquivos Mais Importantes

1. `LightingController.ino`
2. `config.h`
3. `secrets.h`
4. `04_Actuators.ino`
5. `03_WeatherAndSun.ino`
6. `05_Tasks.ino`
7. `01_Utils.ino`
8. `02_Connectivity.ino`

## Riscos Tecnicos

- Credenciais reais em `secrets.h`.
- `WiFiClientSecure::setInsecure()` nao valida certificado TLS.
- O firmware depende do formato e existencia dos dados solares no Firebase.
- A sincronizacao solar diaria depende do loop executar durante o minuto configurado.
- Horario NTP invalido derruba toda a logica de iluminacao, como esperado.
- Alteracoes na curva podem impactar diretamente fauna/flora do aquario.
- Rele e fita sao atuadores fisicos; testar mudancas com cautela.
- Nao ha testes automatizados.

## Resumo Executivo

Firmware ESP32 focado em iluminacao. Ele busca horarios solares diarios no Firebase, valida a ordem dos eventos, calcula fallback de zenite e aplica uma curva suave de brilho e cor numa fita NeoPixel, com rele branco ligado ao redor do zenite. A divisao removeu sensores e gravacao historica deste projeto, deixando essas responsabilidades para o `SensorsController`.
