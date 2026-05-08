****# Transformadores Eletricos — Deep Learning com CRISP-DM

Aplicacao de tecnicas de Deep Learning em dados operacionais de transformadores eletricos para apoiar manutencao preditiva. O projeto cobre tres frentes: deteccao de anomalias, previsao de temperatura do oleo e classificacao do estado operacional.

---

## Sumario

- [Contexto](#contexto)
- [Dataset](#dataset)
- [Metodologia CRISP-DM](#metodologia-crisp-dm)
- [Analise Exploratoria](#analise-exploratoria)
- [Pre-processamento](#pre-processamento)
- [Tarefa 1 — Deteccao de Anomalias (Autoencoder)](#tarefa-1--deteccao-de-anomalias)
- [Tarefa 2 — Previsao de Temperatura (LSTM)](#tarefa-2--previsao-de-temperatura)
- [Tarefa 3 — Classificacao Operacional (Deep MLP)](#tarefa-3--classificacao-operacional)
- [Comparativo dos Modelos](#comparativo-dos-modelos)
- [Conclusoes](#conclusoes)
- [Proximos Passos](#proximos-passos)

---

## Contexto

Transformadores sao equipamentos criticos para o fornecimento de energia eletrica confiavel. Uma falha pode causar interrupcoes prolongadas no abastecimento, danos a outros equipamentos e custos operacionais elevados.

A temperatura do oleo e o principal indicador da condicao termica do equipamento. Monitorar e prever esse indicador permite decisoes proativas de manutencao antes que uma falha ocorra, substituindo a abordagem reativa por uma abordagem preditiva.

---

## Dataset

| Atributo | Valor |
|---|---|
| Total de registros | 17.348 |
| Frequencia | Horaria |
| Periodo | 2016-07-03 a 2018-06-25 |
| Variavel alvo (classificacao) | `class` (0 = normal, 1 = falha) |
| Variavel alvo (regressao) | `OT` — temperatura do oleo |

### Variaveis

| Variavel | Descricao |
|---|---|
| `HUFL` | Carga alta — util |
| `HULL` | Carga alta — total |
| `MUFL` | Carga media — util |
| `MULL` | Carga media — total |
| `LUFL` | Carga baixa — util |
| `LULL` | Carga baixa — total |
| `OT` | Temperatura do oleo |
| `class` | 0 = normal / 1 = falha |

---

## Metodologia CRISP-DM

O projeto seguiu as seis etapas do CRISP-DM (Cross-Industry Standard Process for Data Mining):

```
+-----------------------------+
|   Entendimento do Negocio   |
|  Objetivos e requisitos     |
+-------------+---------------+
              |
+-------------v---------------+
|   Entendimento dos Dados    |
|  EDA, qualidade, padroes    |
+-------------+---------------+
              |
+-------------v---------------+
|    Preparacao dos Dados     |
|  Limpeza, normalizacao      |
+-------------+---------------+
              |
+-------------v---------------+
|         Modelagem           |
|  Autoencoder, LSTM, MLP     |
+-------------+---------------+
              |
+-------------v---------------+
|          Avaliacao          |
|  Metricas e validacao       |
+-------------+---------------+
              |
+-------------v---------------+
|        Implantacao          |
|  Deploy e monitoramento     |
+-----------------------------+
```

---

## Analise Exploratoria

### Desbalanceamento das classes

O principal achado da EDA foi o forte desbalanceamento entre as classes. Apenas 1% dos registros corresponde a falhas reais — o que representa o maior desafio tecnico do projeto.

```
Distribuicao das classes
========================

Normal  ||||||||||||||||||||||||||||||||||||||||  17.175 (99%)
Falha   |                                            173  ( 1%)

         0%                                       100%
```

### Correlacao entre variaveis

A matriz de correlacao revelou que a temperatura do oleo (`OT`) apresenta correlacao mais forte com as variaveis de carga alta e media (`HULL`, `MULL`). A variavel `class` teve baixa correlacao direta com qualquer feature isolada, indicando que o padrao de falha e complexo e nao linearmente separavel.

```
Correlacao com OT (aproximada)
===============================

HULL  |||||||||||||||||||||||||||||||||  alto
MULL  |||||||||||||||||||||||||||||||    alto
HUFL  ||||||||||||||||||||||||||         medio
MUFL  |||||||||||||||||||||              medio
LULL  ||||||||||||||||                   baixo-medio
LUFL  ||||||||||||                       baixo
class |                                  muito baixo
```

Essa observacao justificou o uso de Deep Learning em vez de modelos lineares simples.

---

## Pre-processamento

Foram utilizadas as 7 variaveis numericas do dataset como features:
`HUFL`, `HULL`, `MUFL`, `MULL`, `LUFL`, `LULL`, `OT`

**Decisao sobre outliers:** os outliers nao foram removidos, pois podem representar comportamentos operacionais anomalos — exatamente o que se deseja detectar.

**Estrategia de normalizacao por modelo:**

| Modelo | Normalizacao |
|---|---|
| Autoencoder | MinMaxScaler |
| LSTM | MinMaxScaler |
| Deep MLP | StandardScaler |

---

## Tarefa 1 — Deteccao de Anomalias

### Arquitetura: Autoencoder

O Autoencoder aprende a representacao comprimida dos dados normais e tenta reconstrui-los. Quando o erro de reconstrucao excede um threshold, o ponto e classificado como anomalia.

```
Fluxo do Autoencoder
=====================

Entrada (7 features)
       |
   [Encoder]
   dimensao reduzida
       |
   [Decoder]
   reconstrucao
       |
Erro de reconstrucao
       |
   threshold = percentil 99 do erro no treino
       |
   erro > threshold  -->  ANOMALIA
   erro <= threshold -->  NORMAL
```

**Definicao do threshold:** percentil 99 do erro de reconstrucao no conjunto de treinamento.

### Resultados

| Metrica | Valor |
|---|---|
| Anomalias detectadas | 174 (aprox. 1% dos dados) |
| Threshold | 0.0129 |
| F1-score vs. `class` | 0.012 |

**Observacao importante:** o F1-score baixo nao invalida o modelo. Uma anomalia operacional detectada pelo Autoencoder nao e necessariamente equivalente ao rotulo de falha do dataset. O Autoencoder pode estar identificando comportamentos atipicos que nao chegaram a ser registrados como falha — que e justamente o valor de um sistema preditivo.

---

## Tarefa 2 — Previsao de Temperatura

### Arquitetura: LSTM

Redes LSTM (Long Short-Term Memory) sao adequadas para series temporais por manterem memoria de longo prazo entre os passos da sequencia.

```
Fluxo da LSTM
==============

Janela de entrada: 24 horas de historico
        |
    [ LSTM ]
    processamento sequencial com celulas de memoria
        |
    [ Dense ]
        |
Saida: previsao para t+1h, t+2h, t+3h, t+4h
```

**Divisao temporal dos dados:**

```
|<-------- 70% Treino -------->|<-- 30% Teste -->|
2016-07-03                  ~2017-11          2018-06-25
```

A ordem temporal foi preservada — embaralhar os dados nesse contexto causaria vazamento de informacao do futuro para o treino.

### Resultados

| Metrica | Valor |
|---|---|
| MAE geral | 0.883 |
| RMSE geral | 1.176 |

**Comportamento por horizonte:**

```
Erro por horizonte de previsao
==============================

t+1h  ||||||                    menor erro
t+2h  ||||||||
t+3h  ||||||||||
t+4h  ||||||||||||||||          maior erro

Previsoes mais distantes apresentam maior incerteza — comportamento esperado.
```

---

## Tarefa 3 — Classificacao Operacional

### Arquitetura: Deep MLP

Rede neural profunda com multiplas camadas ocultas para classificar o estado do transformador entre normal e falha.

```
Arquitetura do Deep MLP
========================

[ Entrada: 7 features ]
         |
[ Camada oculta + ReLU ]
         |
[ Dropout — regularizacao ]
         |
[ Camada oculta + ReLU ]
         |
[ Dropout — regularizacao ]
         |
[ Saida: Sigmoid ]
         |
[ Probabilidade de falha (0 a 1) ]
         |
   threshold (ajustado por ROC)
         |
   prob > threshold  -->  FALHA
   prob <= threshold -->  NORMAL
```

**Estrategias para o desbalanceamento:**
- `class_weight`: penaliza erros na classe minoritaria (falha) mais do que na classe majoritaria (normal)
- Threshold ajustado com base na curva ROC, priorizando recall

### Resultados

| Metrica | Valor |
|---|---|
| AUC-ROC | 0.9226 |
| Recall (falha) | 0.9615 |
| Precisao (falha) | 0.0499 |

**Interpretacao do trade-off:**

```
Recall vs. Precisao — contexto de manutencao preditiva
=======================================================

Alto recall (0.96):
  - O modelo detecta 96% das falhas reais
  - Poucos casos de falha passam sem deteccao
  - Prioridade: nao deixar falha passar

Baixa precisao (0.05):
  - Muitos alertas sao falsos positivos
  - Aceitavel: investigar um falso alarme
    e mais barato do que perder uma falha real
```

### Curva ROC (esquematica)

```
1.0 |*
    | **
    |   **
0.8 |     **
    |       *
    |        *
0.6 |         **       AUC = 0.9226
    |           *
    |            **
0.4 |              *
    |               **
    |                 **
0.2 |                   **
    |                     ***
0.0 |________________________*
    0.0  0.2  0.4  0.6  0.8  1.0
         Taxa de Falsos Positivos
```

### Importancia das features (permutacao)

```
Importancia por permutacao — Deep MLP
======================================

OT    ||||||||||||||||||||||||||||||||||||  maior impacto
HULL  |||||||||||||||||||||
HUFL  |||||||||||||||||||
MULL  ||||||||||||||||
MUFL  ||||||||||||||
LULL  ||||||||||
LUFL  |||||||                              menor impacto
```

A temperatura do oleo (`OT`) foi de longe a variavel mais importante, o que e consistente com o conhecimento de dominio: ela reflete diretamente a condicao termica do equipamento.

---

## Comparativo dos Modelos

| Modelo | Tarefa | Metrica principal | Resultado |
|---|---|---|---|
| Autoencoder | Deteccao de anomalias | Anomalias detectadas | 174 (1%) |
| LSTM | Previsao de temperatura | MAE / RMSE | 0.883 / 1.176 |
| Deep MLP | Classificacao de falhas | AUC-ROC / Recall | 0.9226 / 0.9615 |

```
Cobertura dos tres modelos
===========================

AUTOENCODER          LSTM               DEEP MLP
-----------          ----               --------
Monitora             Antecipa           Classifica
comportamentos       temperatura        estado
atipicos             futura             atual
sem rotulos          do oleo            (normal/falha)
     |                   |                  |
     +-------------------+------------------+
                         |
              Sistema integrado de
              manutencao preditiva
```

---

## Conclusoes

Os tres objetivos do projeto foram alcancados:

1. **Deteccao de anomalias** — o Autoencoder identificou 174 pontos atipicos na operacao, com threshold definido pelo percentil 99 do erro de reconstrucao. O modelo e util para alertas precoces, mas nao substitui um classificador supervisionado.

2. **Previsao de temperatura** — a LSTM previu a temperatura do oleo com MAE de 0.88 e RMSE de 1.18, com comportamento de erro crescente conforme o horizonte de previsao aumenta, como esperado.

3. **Classificacao operacional** — o Deep MLP atingiu AUC-ROC de 0.92 e recall de 96% para falhas, priorizando a sensibilidade frente ao forte desbalanceamento de classes (99% normal / 1% falha).

**Principal desafio:** o desbalanceamento extremo das classes condicionou todas as decisoes de modelagem — da escolha de metricas ao ajuste de threshold e uso de class weights.

---

## Proximos Passos

| Direcao | Descricao |
|---|---|
| Mais dados de falha | Coletar mais exemplos da classe minoritaria para melhorar o aprendizado supervisionado |
| LSTM-Autoencoder hibrido | Testar arquitetura combinada para capturar anomalias temporais de forma mais sofisticada |
| Baseline comparativo | Comparar a LSTM com modelos simples (media movel, regressao linear) para validar o ganho do Deep Learning |
| Ajuste de threshold | Calibrar o threshold do MLP com base no custo real de falso positivo versus falso negativo |

---

## Estrutura do Repositorio

```
.
+-- data/
|   +-- transformer_data.csv       # dataset original
+-- notebooks/
|   +-- 01_eda.ipynb               # analise exploratoria
|   +-- 02_autoencoder.ipynb       # deteccao de anomalias
|   +-- 03_lstm.ipynb              # previsao de temperatura
|   +-- 04_mlp.ipynb               # classificacao operacional
+-- src/
|   +-- preprocessing.py           # normalizacao e janelas temporais
|   +-- models.py                  # definicao das arquiteturas
|   +-- evaluate.py                # metricas e visualizacoes
+-- README.md
```

---

## Dependencias

```
tensorflow >= 2.10
scikit-learn >= 1.1
pandas >= 1.5
numpy >= 1.23
matplotlib >= 3.6
seaborn >= 0.12
```

---

## Referencia da Metodologia

CRISP-DM: Chapman, P. et al. (2000). *CRISP-DM 1.0: Step-by-step data mining guide*. SPSS Inc.****
