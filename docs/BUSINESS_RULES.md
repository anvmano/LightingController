# BUSINESS_RULES

## Regra: Horario valido antes de controlar LEDs

### Arquivo

`01_Utils.ino`, `LightingController.ino`, `05_Tasks.ino`

### Metodo

`horarioSincronizado`, `sincronizarHorario`, `setup`, `executarTarefasPeriodicas`

### Objetivo

Evitar uso de data/hora invalida para ler o Firebase e calcular a curva de luz.

### Entradas

- `timeClient.getEpochTime()`
- `MIN_VALID_YEAR`
- status Wi-Fi

### Saidas

- `true` ou `false`
- LEDs/rele desligados quando horario segue invalido
- Retorno antecipado em `executarTarefasPeriodicas()` antes de Firebase e atuadores quando o horario e invalido

### Impacto

Critico.

### O que acontece se for alterada

O firmware pode buscar horarios solares na data errada ou acender o aquario em horarios incorretos.

### Comportamento atual observado

`setup()` tenta conectar Wi-Fi e depois chama `sincronizarHorario()`. Se Wi-Fi estiver desconectado ou o NTP falhar, `sincronizarHorario()` retorna `false` e o boot termina com `desligarLedsAquario()`. No loop, `timeClient.update()` roda apenas com Wi-Fi conectado; enquanto o ano for menor que `MIN_VALID_YEAR`, o firmware tenta nova sincronizacao em intervalo de reconexao e mantem LEDs/rele desligados.

## Regra: Caminho solar padronizado

### Arquivo

`03_WeatherAndSun.ino`

### Metodo

`atualizarHorariosSol`, `montarUrlFirebase`

### Objetivo

Ler horarios solares no caminho `/historico/NascePorDoSol/DD-MM-AAAA`.

### Entradas

- `FIREBASE_HOST`
- `FIREBASE_AUTH`
- data atual formatada

### Saidas

URL REST Firebase no formato `.json?auth=...`.

### Impacto

Critico.

### O que acontece se for alterada

O `LightingController` pode deixar de encontrar os horarios produzidos/gravados por outro componente do sistema.

### Comportamento atual observado

`montarUrlFirebase()` aceita o `FIREBASE_HOST` com ou sem barra final e remove uma barra inicial do caminho recebido. `atualizarHorariosSol()` usa `WiFiClientSecure::setInsecure()`, `HTTPClient`, timeout `FIREBASE_HTTP_TIMEOUT_MS` e so processa o corpo quando o HTTP retorna `HTTP_CODE_OK`.

## Regra: Horarios solares devem ser ordenados

### Arquivo

`03_WeatherAndSun.ino`, `04_Actuators.ino`

### Metodo

`processarJsonHorariosSol`, `horariosSolaresValidos`, `controlarLeds`

### Objetivo

Garantir ordem `amanhecer < nascer < zenite < por do sol < anoitecer`.

### Entradas

Horarios em horas/minutos vindos do Firebase.

### Saidas

- `true` quando a sequencia e valida.
- Desligamento dos LEDs quando a sequencia aplicada e invalida.
- Manutencao dos ultimos horarios validos quando a leitura nova e invalida.
- Aplicacao do ultimo filho valido encontrado no objeto JSON do dia.

### Impacto

Critico.

### O que acontece se for alterada

A curva pode ficar invertida, gerar divisao por intervalo invalido ou acender/desligar em momentos errados.

### Comportamento atual observado

O JSON do Firebase e tratado como objeto com filhos dinamicos. Cada filho que nao for objeto e ignorado. Para filhos objeto, o firmware le os campos solares, calcula minutos do dia, aplica fallback de zenite quando hora ou minuto do zenite esta ausente/negativo, valida a ordem e so entao copia os valores para as variaveis globais. Se nenhum filho valido for encontrado, a chamada retorna `false` e nao substitui valores anteriores.

## Regra: Zenite possui fallback

### Arquivo

`03_WeatherAndSun.ino`, `04_Actuators.ino`

### Metodo

`processarJsonHorariosSol`, `controlarLeds`

### Objetivo

Usar ponto medio entre nascer e por do sol quando zenite vier ausente ou fora da janela esperada.

### Entradas

- nascer do sol
- por do sol
- zenite opcional

### Saidas

Zenite calculado em hora/minuto ou minuto total.

### Impacto

Alto.

### O que acontece se for alterada

O rele branco e o pico da curva de brilho podem falhar quando o Firebase nao trouxer zenite.

### Comportamento atual observado

O fallback aparece em dois pontos. Durante o parsing, zenite ausente ou negativo e substituido pelo ponto medio entre nascer e por do sol antes da validacao. Durante `controlarLeds()`, se o zenite global aplicado estiver fora da janela entre nascer e por do sol, o minuto total de zenite usado na curva tambem cai para o ponto medio. Esse segundo fallback nao regrava as variaveis globais.

## Regra: LEDs desligam em estado inseguro

### Arquivo

`04_Actuators.ino`, `05_Tasks.ino`, `LightingController.ino`

### Metodo

`desligarLedsAquario`, `controlarLeds`, `executarTarefasPeriodicas`, `setup`

### Objetivo

Manter fita e rele desligados quando horario ou horarios solares nao sao confiaveis, e tambem no boot quando Wi-Fi/Firebase impedem obter horario solar valido.

### Entradas

- horario sincronizado
- horarios solares carregados
- validacao da ordem solar
- janela atual da curva

### Saidas

- `brilho = 0`
- LEDs pretos
- `LED_RELAY_PIN` em `LOW`
- `FastLED.show()`

### Impacto

Critico.

### O que acontece se for alterada

Pode deixar iluminacao fisica ligada fora de horario ou manter rele acionado indevidamente.

### Comportamento atual observado

`desligarLedsAquario()` e chamado no boot quando NTP ou leitura solar falham, no loop enquanto o horario e invalido, em `controlarLeds()` quando a ordem solar aplicada e invalida e quando o minuto atual esta fora da janela de luz. O loop usa uma flag para evitar repetir o desligamento continuamente enquanto o horario segue invalido.

Perda de Wi-Fi depois que horario e horarios solares validos ja existem nao desliga a iluminacao por si so. Nesse caso, o loop tenta reconectar e a curva continua usando o ultimo estado valido enquanto `horarioSincronizado()` permanecer verdadeiro.

## Regra: Curva de brilho por fases solares

### Arquivo

`04_Actuators.ino`

### Metodo

`controlarLeds`, `interpolarBrilho`, `suavizarTransicao`

### Objetivo

Simular aumento e reducao gradual de luminosidade ao longo do dia.

### Entradas

- minuto atual
- inicio de luz
- nascer
- zenite
- por do sol
- fim de luz
- constantes de brilho

### Saidas

Variavel global `brilho` e atualizacao da fita.

### Impacto

Alto.

### O que acontece se for alterada

Muda fotoperiodo e intensidade luminosa do aquario.

### Comportamento atual observado

A interpolacao usa `suavizarTransicao()` com curva smoothstep. As fases sao: pre-nascer ate `LED_SUNRISE_BRIGHTNESS`, nascer ate zenite chegando em `LED_PLANTED_TANK_MAX_BRIGHTNESS`, zenite ate por do sol descendo para `LED_SUNSET_BRIGHTNESS`, e pos-por do sol descendo para zero. O brilho final e limitado por `MAX_PWM_VALUE`.

## Regra: Janela de luz respeita pre-nascer e pos-por do sol

### Arquivo

`04_Actuators.ino`

### Metodo

`controlarLeds`

### Objetivo

Acender antes do nascer e manter um periodo curto apos o por do sol, sem ultrapassar amanhecer/anoitecer.

### Entradas

- `LED_PRE_SUNRISE_MINUTES`
- `LED_POST_SUNSET_MINUTES`
- amanhecer
- nascer
- por do sol
- anoitecer

### Saidas

Minuto inicial e final da janela de luz.

### Impacto

Alto.

### O que acontece se for alterada

Pode alongar ou reduzir o fotoperiodo real aplicado ao aquario.

## Regra: Rele branco liga ao redor do zenite

### Arquivo

`04_Actuators.ino`

### Metodo

`controlarLeds`

### Objetivo

Ligar a regua LED branca em janela centralizada no zenite.

### Entradas

- minuto total de zenite
- `LED_RELAY_WINDOW_MINUTES`
- minuto atual

### Saidas

`digitalWrite(LED_RELAY_PIN, HIGH/LOW)`.

### Impacto

Alto.

### O que acontece se for alterada

A regua branca pode permanecer ligada/desligada em periodo inadequado.

## Regra: Cores por fileira para aquario plantado

### Arquivo

`04_Actuators.ino`

### Metodo

`aplicarCoresCicloSolar`

### Objetivo

Distribuir espectros por fileira: branco, vermelho, azul, neutro e ambar.

### Entradas

- brilho base
- minuto atual
- nascer
- zenite
- por do sol
- layout `LED_ROWS` x `LEDS_PER_ROW`

### Saidas

Valores HSV nos 125 LEDs.

### Impacto

Medio/alto.

### O que acontece se for alterada

Altera a aparencia e o espectro visual da iluminacao.

### Comportamento atual observado

O layout logico usa `i % LEDS_PER_ROW` para coluna e `(i / LEDS_PER_ROW) % LED_ROWS` para fileira. As fileiras aplicam branco, vermelho, azul, neutro e ambar. As duas primeiras e duas ultimas colunas recebem reducao com `nscale8(230)`.

# REGRAS QUE NAO DEVEM SER ALTERADAS

- Desligar LEDs/rele quando horario e invalido.
- Validar ordem dos horarios solares antes de aplicar a curva.
- Manter caminho `/historico/NascePorDoSol/DD-MM-AAAA`.
- Preservar fallback de zenite.
- Nao expor `secrets.h`.
- Manter rele desligado fora da janela configurada.

# MAPA DE IMPACTO

## Arquivo: LightingController.ino

Responsabilidade: includes, globais, boot e loop.

Nivel de risco: Critico.

## Arquivo: config.h

Responsabilidade: pinos, intervalos, brilho e agenda.

Nivel de risco: Critico para hardware; alto para fotoperiodo.

## Arquivo: secrets.h

Responsabilidade: credenciais Wi-Fi e Firebase.

Nivel de risco: Critico e sensivel.

## Arquivo: 03_WeatherAndSun.ino

Responsabilidade: contrato de leitura dos horarios solares.

Nivel de risco: Alto.

## Arquivo: 04_Actuators.ino

Responsabilidade: comportamento fisico dos LEDs e rele.

Nivel de risco: Alto.

## Arquivo: 05_Tasks.ino

Responsabilidade: ordem e frequencia das tarefas periodicas.

Nivel de risco: Alto.

# CODIGO REMOVIDO PELA DIVISAO

As regras de sensores, AirQuality, Aquario, MQ135, BME680, DS18B20, TDS, pH, turbidez, fan e gravacao historica nao pertencem mais a este firmware. Consulte `SensorsController` para essas responsabilidades.
