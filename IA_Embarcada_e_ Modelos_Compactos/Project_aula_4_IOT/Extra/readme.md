# **Atividade Avaliativa Prática (4/6)**

## Conteúdo principal

### Condições de conclusão

1. Reproduza os passos do **Hello World**;
2. Print da tela do **Wokwi** rodando o **Hello World**;
3. Análise do código e documentação em um breve relatório sobre as observações encontradas.

### Extra (+1 ponto nas atividades práticas)

* Modifique o código para uma nova aplicação, com novo sensor e novo dataset;
* Não pode ser um outro exemplo do **esp-tflite-micro**, exceto na condição de realizar um novo retreino/finetuning do modelo para novas funcionalidades.



# Sistema Inteligente de Monitoramento de Datacenter com ESP32-S3

## Atividade Avaliativa Prática (4/6)

Este projeto foi desenvolvido como parte da **Atividade Avaliativa Prática 4/6** da disciplina de **IA Embarcada e Modelos Compactos**.

A aplicação parte dos conceitos apresentados no exemplo inicial de **Hello World** e evolui para uma aplicação própria de **monitoramento inteligente de um ambiente de datacenter**, utilizando sensores, displays, LEDs, buzzer e um modelo de Inteligência Artificial embarcado no **ESP32-S3**.

Além da lógica tradicional de monitoramento, o projeto utiliza um modelo de IA treinado especificamente para classificar diferentes condições do ambiente em:

- **NORMAL**
- **ATENÇÃO**
- **CRÍTICO**

O projeto também possui um **modo automático de demonstração**, no qual diferentes cenários são gerados aleatoriamente para permitir a avaliação contínua do modelo de IA.

---

## 1. Objetivos da atividade

A atividade possui como objetivos principais:

1. Reproduzir os passos do exemplo **Hello World** utilizando ESP-IDF;
2. Executar e validar a aplicação no ambiente de simulação **Wokwi**;
3. Analisar o funcionamento do código e documentar as observações;
4. Desenvolver uma nova aplicação utilizando sensores;
5. Criar um novo conjunto de dados (**dataset**);
6. Treinar um novo modelo de Inteligência Artificial;
7. Converter o modelo para um formato adequado à execução embarcada;
8. Executar a inferência diretamente no ESP32-S3.

A aplicação desenvolvida não utiliza simplesmente um exemplo pronto do `esp-tflite-micro`. Foi desenvolvido um novo dataset e um novo modelo voltado ao problema de monitoramento de datacenter.

---

## 2. Descrição do projeto

O sistema simula uma solução de monitoramento para um pequeno ambiente de datacenter.

O ESP32-S3 realiza a aquisição de informações relacionadas a:

- temperatura ambiente;
- umidade ambiente;
- temperatura dos equipamentos;
- umidade próxima aos equipamentos;
- concentração simulada de gás/fumaça;
- presença ou ausência de energia.

Essas informações são utilizadas tanto pela lógica tradicional do sistema quanto pelo modelo de Inteligência Artificial.

A aplicação possui dois modos de operação.

### Modo Manual

No modo manual, o sistema utiliza os valores dos sensores disponíveis no ambiente de simulação.

As alterações realizadas nos sensores são processadas pelo ESP32-S3 em tempo real.

### Modo Automático

No modo automático, o sistema gera diferentes cenários de datacenter utilizando valores aleatórios dentro de faixas previamente definidas.

Os cenários simulam situações:

- normais;
- de atenção;
- críticas.

A cada ciclo, os valores são enviados para o modelo de IA, permitindo observar no monitor serial várias classificações consecutivas.

# Projeto Extra
### Comprovação Visual no Simulador (Wokwi)

> **Observação para entrega:**
> 

https://github.com/user-attachments/assets/090968d5-dac6-4ba3-b65c-3a1873a2672d

> 
> *(Gravação do Hello World no simulador Wokwi)*



Esse modo foi desenvolvido principalmente para demonstrar o funcionamento da inferência embarcada e verificar o comportamento do modelo diante de diferentes condições.

---

## 3. Arquitetura da solução

O funcionamento geral pode ser representado da seguinte forma:

```text
                    +----------------------+
                    |      ESP32-S3        |
                    |                      |
                    |  Sistema embarcado   |
                    +----------+-----------+
                               |
              +----------------+----------------+
              |                |                |
              v                v                v
        Sensores físicos   Simulação       Energia
        DHT22 / MQ2       automática       simulada
              |                |                |
              +----------------+----------------+
                               |
                               v
                    +----------------------+
                    | Tratamento dos dados |
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    |      Modelo IA       |
                    |      V1 - INT8       |
                    +----------+-----------+
                               |
                 +-------------+-------------+
                 |             |             |
                 v             v             v
              NORMAL        ATENÇÃO       CRÍTICO
                 |             |             |
                 +-------------+-------------+
                               |
             +-----------------+-----------------+
             |                 |                 |
             v                 v                 v
          OLED/LCD          LEDs             Buzzer
```

---

## 4. Componentes utilizados

### Microcontrolador

- ESP32-S3 DevKitC-1

### Sensores

- 2x DHT22
- Sensor de gás/fumaça simulado
- Botão para simulação de energia

### Displays

- 2x OLED SSD1306
- 1x LCD 20x4 com interface I2C

### Atuadores

- LED vermelho
- LED amarelo
- LED verde
- Buzzer

---

## 5. Mapeamento de hardware

A aplicação utiliza o seguinte mapeamento:

| Componente | GPIO / Endereço | Função |
|---|---:|---|
| DHT22 ambiente | GPIO 4 | Temperatura e umidade ambiente |
| DHT22 equipamentos | GPIO 16 | Temperatura e umidade dos equipamentos |
| I2C SDA | GPIO 8 | Comunicação I2C |
| I2C SCL | GPIO 9 | Clock I2C |
| Sensor de gás | GPIO 7 | Leitura analógica |
| Buzzer | GPIO 14 | Alarme sonoro |
| LED vermelho | GPIO 10 | Estado crítico |
| LED amarelo | GPIO 11 | Estado de atenção |
| LED verde | GPIO 12 | Estado normal |
| Botão de energia | GPIO 17 | Simulação de energia |
| OLED 1 | 0x3C | Informações do sistema |
| OLED 2 | 0x3D | Informações da IA |
| LCD 20x4 | 0x27 | Informações gerais |

---

## 6. Regras de monitoramento

O sistema possui regras para identificar situações de risco.

### Temperatura

| Condição | Estado |
|---|---|
| < 17 °C | Temperatura baixa |
| 17 °C até 22 °C | Normal |
| > 22 °C | Atenção |
| > 24 °C | Crítico |

Além disso, o sistema acompanha o tempo em que a temperatura permanece acima dos limites.

Quando a temperatura dos equipamentos permanece acima de **22 °C durante 30 minutos**, o sistema considera a possibilidade de um problema relacionado à climatização.

---

## 7. Monitoramento de gás

O sensor de gás é utilizado como um indicador simulado para representar uma possível presença de fumaça ou gás.

Os limites utilizados são:

| Valor ADC | Estado |
|---:|---|
| < 1800 | Normal |
| 1800–2799 | Atenção |
| >= 2800 | Crítico |

> **Observação:** o sensor utilizado no Wokwi possui finalidade didática e de simulação. Ele não representa um detector de fumaça ou gás certificado para utilização em um datacenter real.

---

## 8. Monitoramento de energia

A alimentação também é simulada através de um botão.

O sistema considera:

```text
GPIO 17 = HIGH → Energia presente
GPIO 17 = LOW  → Energia ausente
```

Quando ocorre uma queda de energia, o sistema registra:

- ocorrência da queda;
- duração da queda;
- quantidade de quedas;
- momento do restabelecimento.

O estado de falta de energia é tratado separadamente da classificação do modelo de IA.

---

## 9. Inteligência Artificial embarcada

A principal diferença desta aplicação em relação ao exemplo básico é a utilização de um novo modelo de Inteligência Artificial treinado especificamente para o problema proposto.

O modelo foi desenvolvido utilizando um novo dataset relacionado às condições simuladas de um ambiente de datacenter.

### Entradas do modelo

O modelo utiliza seis características:

```text
1. Temperatura ambiente
2. Umidade ambiente
3. Temperatura dos equipamentos
4. Umidade dos equipamentos
5. Valor do sensor de gás
6. Estado da energia
```

Representação:

```text
[Temp_Amb,
 Umid_Amb,
 Temp_Rack,
 Umid_Rack,
 Gas,
 Energia]
```

---

## 10. Classes do modelo

O modelo possui três classes:

| Classe | Código | Significado |
|---|---:|---|
| NORMAL | 0 | Condições normais |
| ATENÇÃO | 1 | Condições que exigem acompanhamento |
| CRÍTICO | 2 | Condições críticas |

A falta de energia não é uma classe da IA. Ela é identificada diretamente pela lógica do sistema embarcado.

---

## 11. Dataset

Foi desenvolvido um novo dataset específico para esta aplicação.

O conjunto de dados possui:

```text
900 registros
3 classes
300 registros por classe
```

Distribuição:

```text
NORMAL  → 300
ATENÇÃO → 300
CRÍTICO → 300
```

Divisão utilizada:

```text
Treinamento → 630 registros
Validação   → 135 registros
Teste       → 135 registros
```

A divisão foi realizada mantendo o equilíbrio entre as três classes.

---

## 12. Treinamento do modelo

A arquitetura utilizada no modelo foi:

```text
Entrada: 6 características
        |
        v
Dense(16) + ReLU
        |
        v
Dense(8) + ReLU
        |
        v
Dense(3) + Softmax
```

O treinamento utilizou:

- TensorFlow/Keras;
- StandardScaler;
- Adam;
- Sparse Categorical Crossentropy;
- 100 épocas;
- batch size 32.

O modelo final apresentou:

```text
Loss:     0.0842
Accuracy: 97.78%
```

---

## 13. Avaliação do modelo

O conjunto de teste possui 135 amostras.

Resultado obtido:

```text
131 classificações corretas
4 classificações incorretas

Acurácia no teste: 97,04%
```

Matriz de confusão:

```text
                 Predito
              N     A     C

Real N       44    1     0
Real A        2   43     0
Real C        0    0    45
```

Onde:

```text
N = NORMAL
A = ATENÇÃO
C = CRÍTICO
```

O resultado demonstra que o modelo conseguiu diferenciar as três condições utilizadas no experimento, incluindo a identificação das situações críticas presentes no conjunto de teste.

---

## 14. Modelo otimizado para o ESP32-S3

Após o treinamento, o modelo foi convertido para **TensorFlow Lite Micro** utilizando quantização **INT8**.

O objetivo da conversão foi reduzir o tamanho do modelo e permitir sua execução diretamente no microcontrolador.

Informações do modelo embarcado:

```text
Formato: TensorFlow Lite Micro
Quantização: INT8
Tamanho aproximado: 3,5 KB
Entradas: 6
Saídas: 3
```

O modelo convertido está integrado ao projeto como:

```text
main/model/modelo_v1_int8.cc
```

---

## 15. Inferência no ESP32-S3

A inferência é executada diretamente no ESP32-S3.

O fluxo é:

```text
Sensores
   |
   v
Leitura dos valores
   |
   v
Pré-processamento
   |
   v
Quantização INT8
   |
   v
TensorFlow Lite Micro
   |
   v
Classificação
   |
   v
NORMAL / ATENÇÃO / CRÍTICO
```

O ESP32-S3 não precisa enviar os dados para um computador ou servidor para realizar a classificação.

---

## 16. Modo automático

O modo automático foi desenvolvido para facilitar a demonstração da Inteligência Artificial.

A cada ciclo, o sistema seleciona aleatoriamente um cenário.

### Cenário NORMAL

Valores gerados dentro de faixas próximas às condições normais:

```text
Temperatura ambiente: aproximadamente 18–21 °C
Umidade:              aproximadamente 45–60 %
Temperatura rack:     aproximadamente 18,5–21,5 °C
Gás:                  aproximadamente 500–1299
Energia:              presente
```

### Cenário ATENÇÃO

Valores representando uma situação de atenção:

```text
Temperatura ambiente: aproximadamente 21,5–23 °C
Umidade:              aproximadamente 55–70 %
Temperatura rack:     aproximadamente 22,2–23,8 °C
Gás:                  aproximadamente 1900–2599
Energia:              presente
```

### Cenário CRÍTICO

Valores representando uma situação crítica:

```text
Temperatura ambiente: aproximadamente 24,5–29 °C
Umidade:              aproximadamente 65–85 %
Temperatura rack:     aproximadamente 25–30 °C
Gás:                  aproximadamente 3000–4095
Energia:              presente
```

A finalidade desse modo é permitir observar continuamente diferentes entradas sendo avaliadas pelo modelo.

---

## 17. Exemplo de funcionamento

Exemplo simplificado do fluxo apresentado no monitor serial:

```text
========================================
MODO AUTOMATICO
Cenario: NORMAL

Temp ambiente: 19.4 C
Umid ambiente: 52.3 %
Temp rack:     20.1 C
Umid rack:     54.8 %
Gas:           842
Energia:       PRESENTE

Classe IA: IA: NORMAL
Probabilidade:
NORMAL: 99.6 %
ATENCAO: 0.3 %
CRITICO: 0.1 %

Status: NORMAL
========================================
```

Outro ciclo:

```text
========================================
MODO AUTOMATICO
Cenario: ATENCAO

Temp ambiente: 22.7 C
Umid ambiente: 63.2 %
Temp rack:     23.1 C
Umid rack:     61.4 %
Gas:           2145
Energia:       PRESENTE

Classe IA: IA: ATENCAO
Probabilidade:
NORMAL: 2.1 %
ATENCAO: 96.8 %
CRITICO: 1.1 %

Status: ATENCAO
========================================
```

E um cenário crítico:

```text
========================================
MODO AUTOMATICO
Cenario: CRITICO

Temp ambiente: 27.3 C
Umid ambiente: 76.1 %
Temp rack:     28.4 C
Umid rack:     72.5 %
Gas:           3487
Energia:       PRESENTE

Classe IA: IA: CRITICO
Probabilidade:
NORMAL: 0.0 %
ATENCAO: 0.4 %
CRITICO: 99.6 %

Status: CRITICO
========================================
```

Os valores apresentados acima são exemplos de execução.

---

## 18. Interface visual

O sistema utiliza três interfaces principais para apresentar as informações.

### OLED 1

Apresenta informações relacionadas ao estado atual do datacenter, como:

- temperatura;
- umidade;
- gás;
- energia;
- estado do sistema.

### OLED 2

É utilizado para apresentar informações relacionadas à análise da Inteligência Artificial.

Exemplo:

```text
IA ANALISE

NORMAL
98.7%

ATENCAO
1.2%

CRITICO
0.1%
```

### LCD 20x4

O LCD apresenta informações complementares do sistema e possui páginas de visualização.

---

## 19. Sistema de alarmes

O projeto utiliza diferentes formas de indicação.

### LED verde

Indica condição normal.

### LED amarelo

Indica condição de atenção.

### LED vermelho

Indica condição crítica.

### Buzzer

É utilizado para indicar situações que exigem atenção ou alarme.

---

## 20. Estrutura do projeto

Estrutura principal:

```text
Project_aula_4_IOT/
│
├── main/
│   ├── main.c
│   ├── datacenter.c
│   ├── datacenter.h
│   │
│   ├── tflite_runner_v1.cc
│   ├── tflite_runner_v1.h
│   │
│   ├── tflite_runner.cc
│   ├── tflite_runner.h
│   │
│   ├── preprocessamento.c
│   ├── preprocessamento.h
│   │
│   ├── mpu6050.c
│   ├── mpu6050.h
│   │
│   ├── oled.c
│   ├── oled.h
│   │
│   ├── buzzer.c
│   ├── buzzer.h
│   │
│   ├── model/
│   │   └── modelo_v1_int8.cc
│   │
│   └── CMakeLists.txt
│
├── CMakeLists.txt
├── diagram.json
├── wokwi.toml
├── sdkconfig
└── README.md
```

---

## 21. Tecnologias utilizadas

- **C**
- **C++**
- **ESP-IDF**
- **ESP32-S3**
- **TensorFlow**
- **TensorFlow Lite Micro**
- **Keras**
- **Python**
- **Wokwi**
- **VS Code**
- **Docker**
- **DHT22**
- **SSD1306 OLED**
- **LCD 20x4**
- **I2C**
- **ADC**

---

## 22. Dependências

O projeto utiliza o componente:

```text
esp-idf-lib/dht
```

Versão utilizada:

```text
^1.2.0
```

Também utiliza:

```text
espressif/esp-tflite-micro
```

A dependência do DHT pode ser instalada com:

```powershell
idf.py add-dependency "esp-idf-lib/dht^1.2.0"
```

Depois:

```powershell
idf.py reconfigure
```

---

## 23. Compilação

Com o ambiente ESP-IDF configurado, entre na pasta do projeto.

Exemplo:

```powershell
cd D:\Residencias\UniSenai\Pos\IA_Embarcada_e_Modelos_Compactos\atv4\Project_aula_4_IOT
```

Configure o alvo:

```powershell
idf.py set-target esp32s3
```

Compile:

```powershell
idf.py build
```

Se a compilação terminar corretamente, o projeto estará pronto para ser executado.

---

## 24. Execução no Wokwi

O projeto pode ser executado utilizando o **Wokwi integrado ao VS Code**.

O arquivo:

```text
wokwi.toml
```

é utilizado para indicar os arquivos necessários para a simulação.

Exemplo:

```toml
[wokwi]
version = 1
elf = "build/Project_aula_4_IOT.elf"
firmware = "build/flasher_args.json"
```

Após realizar o build:

1. Abra o projeto no VS Code;
2. Abra o Wokwi;
3. Inicie a simulação;
4. Abra o monitor serial;
5. Observe as leituras dos sensores;
6. Observe as classificações da IA;
7. Teste o modo manual;
8. Teste o modo automático.

---

## 25. Demonstração

### Execução no VS Code

<!-- Adicionar aqui o vídeo da execução -->

### Execução no Wokwi

<!-- Adicionar aqui as imagens da execução -->

Exemplo:

```markdown
![Sistema funcionando](imagens/wokwi.png)
```

### Link do Wokwi

<!-- Adicionar aqui o link do projeto Wokwi -->

---

## 26. Análise da aplicação

A atividade permitiu observar a diferença entre uma aplicação embarcada convencional e uma aplicação que utiliza Inteligência Artificial.

Na abordagem convencional, as condições são verificadas utilizando limites definidos diretamente no código.

Por exemplo:

```text
Temperatura > 24 °C
        ↓
Estado crítico
```

Com a inclusão da IA, os diferentes sensores são analisados conjuntamente pelo modelo.

```text
Temperatura
Umidade
Gás
Energia
    ↓
Modelo IA
    ↓
Classificação
```

Isso permite que diferentes características sejam consideradas simultaneamente durante a classificação.

A execução no próprio ESP32-S3 também demonstra a possibilidade de utilizar modelos compactos de IA em dispositivos com recursos limitados.

---

## 27. Observações sobre o experimento

Durante o desenvolvimento foram observados alguns pontos importantes:

- o tamanho do modelo precisa ser compatível com a memória disponível;
- a quantização INT8 reduz significativamente o tamanho do modelo;
- o pré-processamento dos dados precisa ser mantido de forma consistente entre treinamento e inferência;
- os mesmos padrões utilizados durante o treinamento precisam ser respeitados durante a execução embarcada;
- sensores simulados permitem testar diferentes condições sem a necessidade de equipamentos físicos;
- o modo automático facilita a demonstração de várias classificações consecutivas;
- a lógica tradicional continua sendo importante para condições que não fazem parte das classes do modelo, como a falta de energia.

---

## 28. Limitações

Este projeto possui finalidade acadêmica e experimental.

As informações geradas no Wokwi são simuladas e não representam necessariamente as condições de um datacenter real.

O sensor de gás utilizado no ambiente de simulação não deve ser considerado um sistema certificado de detecção de incêndio ou gases.

Da mesma forma, o botão utilizado para representar energia simula uma queda de alimentação e não representa uma instalação elétrica real.

Para uma aplicação real seriam necessários:

- sensores industriais;
- sistemas de alimentação apropriados;
- sensores certificados;
- redundância;
- armazenamento histórico;
- comunicação com sistemas de gerenciamento;
- mecanismos de segurança;
- validação em ambiente real.

---

## 29. Conclusão

O projeto desenvolvido atende à proposta da **Atividade Avaliativa Prática 4/6** ao partir de uma aplicação básica em ESP32-S3 e evoluí-la para uma aplicação própria utilizando sensores e Inteligência Artificial.

Foi desenvolvido um novo dataset, realizado o treinamento de um modelo específico para o problema de monitoramento de datacenter e posteriormente realizada sua conversão para **TensorFlow Lite Micro com quantização INT8**.

O modelo apresentou **97,04% de acurácia no conjunto de teste**, com 131 classificações corretas em 135 amostras.

A execução no ESP32-S3 demonstra que é possível realizar a inferência localmente, sem depender de um computador ou servidor externo para a classificação.

O modo automático também permite gerar diferentes condições simuladas de funcionamento, facilitando a demonstração do comportamento da IA diante de situações normais, de atenção e críticas.

Dessa forma, o projeto integra conceitos de:

```text
Sensoriamento
      +
Sistemas embarcados
      +
Processamento de dados
      +
Machine Learning
      +
TensorFlow Lite Micro
      +
IA embarcada
```

formando uma aplicação experimental de monitoramento inteligente executada diretamente no ESP32-S3.
