#  Projeto Final

## UC: IA Embarcada e Modelos Compactos

### Desenvolva uma aplicação de IA Embarcada no qual os seguintes passos são
realizados:
• Coleta de dados de sensores;

• Treinamento de um modelo com dataset público ou próprio coletado;

• Conversão e compressão do modelo para embarcá-lo;

• Desenvolvimento da pipeline de inferência no dispositivo desde a leitura dos dados
até a inferência do modelo.

### Orientações gerais:

• O hardware principal usado para a disciplina é o ESP32-S3, porém se houver interesse do
aluno, pode ser desenvolvido modelos para outros hardwares, como por exemplo:

Raspberry Pi Pico, Arduino, STM32;

• Pode ser utilizado tanto o hardware físico ou simulado na plataforma Wokwi;

• O projeto final não se limita a TinyML, pode ser desenvolvido aplicações para Edge Devices
também como Raspberry Pi, Android / iOS Mobile, entre outros.

• Nesse caso, o sensor pode ser uma câmera, microfone, acelerômetro, entre outras opções;

• Ainda deve ser realizado um treinamento ou finetuning de modelo, compressão e deploy em
dispositivo (real ou simulado);

### Exemplos de datasets públicos para o desenvolvimento do projeto:

• Google Speech Commands Dataset;

• Visual Wake Words (VWW) Dataset;

• UCI Human Activity Recognition (HAR);

• entre outros; 


# Sistema de Monitoramento e Predição de Parâmetros de Biorremediação com Inteligência Artificial Embarcada

## 1. O problema

O tratamento biológico de efluentes é um processo complexo, no qual a eficiência da remoção de poluentes depende de diferentes variáveis físicas, químicas e biológicas. Para avaliar a qualidade do efluente tratado, é necessário analisar parâmetros como a Demanda Química de Oxigênio (DQO), o nitrogênio, o fósforo e o enxofre, conforme as características do processo e as exigências ambientais aplicáveis.

Essas análises podem exigir coleta de amostras, procedimentos laboratoriais e tempo para obtenção dos resultados. A demora na disponibilidade dessas informações dificulta o acompanhamento contínuo do tratamento e pode atrasar decisões operacionais.

Esse cenário apresenta três desafios principais:

1. **Demora na obtenção dos resultados:** a dependência de análises laboratoriais pode dificultar o acompanhamento frequente da evolução do tratamento.
2. **Custos operacionais:** a realização recorrente de análises e a manutenção do processo por períodos maiores que os necessários podem elevar os custos.
3. **Risco ambiental e regulatório:** decisões de descarte sem evidências suficientes sobre a qualidade do efluente podem resultar em impactos ambientais e no descumprimento das exigências legais.

## 2. A solução proposta

O projeto propõe um sistema inteligente de monitoramento e predição de parâmetros de biorremediação utilizando Inteligência Artificial embarcada no microcontrolador ESP32-S3.

A solução funciona como um **sistema de sensores virtuais**, estimando parâmetros de difícil medição contínua a partir de variáveis obtidas por sensores físicos e do tempo decorrido de tratamento.

### 2.1. Aquisição de dados

O sistema coleta informações por meio de sensores de baixo custo, incluindo:

* **pH:** indicador das condições químicas do meio.
* **Turbidez:** medida indireta relacionada à presença de partículas em suspensão.
* **Sinal do sensor de gás:** variável auxiliar para o acompanhamento experimental, cuja interpretação depende das características e da calibração do sensor.
* **Temperatura:** variável relevante para a análise das condições do processo biológico.
* **Tempo de tratamento:** informação utilizada para acompanhar a evolução do processo.

Essas variáveis são utilizadas como entradas para o modelo de Inteligência Artificial.

### 2.2. Inteligência Artificial na borda (*Edge AI*)

O modelo de aprendizado de máquina é executado localmente no ESP32-S3, permitindo realizar inferências diretamente no dispositivo, sem depender de conexão permanente com a internet.

A implementação em C possibilita integrar a inferência ao firmware responsável pela aquisição dos sensores, pelo processamento dos dados e pela apresentação dos resultados nas telas OLED.

Essa arquitetura busca oferecer baixo custo computacional, resposta rápida e maior autonomia operacional.

### 2.3. Predição de parâmetros do efluente

A partir das variáveis de entrada, o modelo estima as concentrações de parâmetros associados à qualidade do efluente, incluindo:

* Demanda Química de Oxigênio (DQO);
* Nitrogênio (N);
* Fósforo (P);
* Enxofre (S).

As estimativas permitem acompanhar tendências durante o tratamento e apoiar a avaliação de sua evolução.

A confiabilidade dessas previsões depende da representatividade dos dados utilizados no treinamento, da validação experimental e do desempenho do modelo em condições reais de operação.

### 2.4. Monitoramento e apoio à decisão ambiental

Os resultados podem ser apresentados em telas OLED conectadas ao microcontrolador e em um painel de monitoramento desenvolvido com Streamlit.

A aplicação pode comparar as estimativas com os critérios ambientais pertinentes, sinalizando condições que merecem atenção e indicando quando são necessárias análises complementares.

A verificação de conformidade deve considerar a legislação aplicável ao tipo de efluente e ao seu destino, incluindo os requisitos pertinentes do Conselho Nacional do Meio Ambiente (CONAMA), além de eventuais normas estaduais e condicionantes da licença ambiental.

**A indicação gerada pela IA é um recurso de apoio à decisão, e não uma certificação automática de que o efluente está apto para descarte.** A confirmação da conformidade exige evidências e procedimentos de validação adequados, incluindo análises laboratoriais quando aplicáveis.

## 3. Impacto esperado

A proposta busca transformar o acompanhamento do tratamento de efluentes em um processo mais frequente, acessível e orientado por dados.

Entre os benefícios esperados estão:

* **Monitoramento contínuo:** acompanhamento da evolução do processo sem depender exclusivamente de análises laboratoriais pontuais.
* **Redução potencial de custos:** possibilidade de otimizar a frequência das análises e identificar oportunidades de melhoria operacional.
* **Apoio à tomada de decisão:** identificação de tendências e de situações que exijam investigação ou intervenção.
* **Processamento local:** execução da IA diretamente no dispositivo, com menor dependência de infraestrutura em nuvem.
* **Apoio à gestão ambiental:** organização de informações que contribuam para a avaliação do tratamento e para o acompanhamento dos requisitos regulatórios.

Esses benefícios deverão ser avaliados por meio de testes experimentais, comparações com resultados laboratoriais e análises do desempenho do sistema.

## 4. Diferencial tecnológico

O principal diferencial do projeto está na integração entre sensores físicos, modelo preditivo embarcado e interface de monitoramento em uma arquitetura de baixo custo.

Em vez de se limitar à coleta de dados ou à visualização de indicadores, o sistema utiliza Inteligência Artificial para estimar parâmetros que não são medidos diretamente pelos sensores instalados.

Dessa forma, a solução explora o conceito de sensores virtuais (*soft sensors*) e de IA na borda para ampliar a capacidade de acompanhamento de processos de biorremediação.

## 5. Resumo

O projeto investiga como a Inteligência Artificial embarcada pode contribuir para o monitoramento inteligente de processos de biorremediação, estimando parâmetros relevantes da qualidade do efluente a partir de variáveis físicas e químicas de aquisição acessível.

Ao combinar sensores, inferência local e visualização dos resultados, a proposta busca oferecer uma ferramenta de apoio à análise da evolução do tratamento, à identificação de tendências e à tomada de decisão operacional.

O objetivo é demonstrar, por meio de validação experimental, o potencial de uma solução acessível e autônoma para complementar os métodos convencionais de monitoramento ambiental.


# Resumo técnico 

| Pergunta                          | Resposta                             |
| --------------------------------- | ------------------------------------ |
| Qual modelo?                      | Rede Neural Artificial MLP           |
| Tipo de problema?                 | Regressão                            |
| Quantas entradas?                 | 5                                    |
| Entradas?                         | Dia, pH, temperatura, turbidez e CO₂ |
| Quantas camadas ocultas?          | 2                                    |
| Neurônios por camada?             | 8 + 8                                |
| Ativação?                         | ReLU nas ocultas                     |
| Saída?                            | 4 valores contínuos                  |
| Saídas?                           | DQO, P, N e S                        |
| Otimizador?                       | Adam                                 |
| Loss?                             | MSE                                  |
| Épocas?                           | 500                                  |
| Amostras na etapa de treinamento? | 16                                   |
| Teste separado?                   | Não                                  |
| Validação separada?               | Não                                  |
| Onde treinou?                     | Computador/Keras                     |
| Onde executa a inferência?        | ESP32-S3                             |
| Linguagem embarcada?              | C                                    |
| Estratégia?                       | Edge AI                              |
| O ESP32 treina?                   | Não                                  |
| O ESP32 faz inferência?           | Sim                                  |
| Saída é classificação?            | Não                                  |
| Saída é regressão?                | Sim                                  |

