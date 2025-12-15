# Projeto de Engenharia de Dados: Construção de um Pipeline de Integração Financeira



## Resumo Executivo

Este documento detalha a proposta de um projeto de **Engenharia de Dados** focado na construção de um *pipeline* robusto para a integração e harmonização de dados financeiros provenientes de três fontes primárias: a **Comissão de Valores Mobiliários (CVM)**, o **Yahoo Finance (YFinance)** e a **B3 (Brasil, Bolsa, Balcão)**. O objetivo principal é transformar dados heterogêneos — que variam em granularidade, estrutura e semântica — em um *dataset* analítico unificado, pronto para consumo em aplicações de *Business Intelligence* (BI) e modelos de *Machine Learning* (ML). O projeto é estruturado em fases claras de ingestão, tratamento, harmonização e armazenamento, garantindo a qualidade e a reprodutibilidade da informação.

## 1. Introdução e Justificativa

A análise de risco e o estudo de desempenho no mercado de capitais brasileiro exigem a combinação de informações de natureza distinta. Dados contábeis, que refletem a saúde estrutural de uma companhia, precisam ser correlacionados com dados de mercado, que capturam o comportamento de preços e a liquidez. A dificuldade reside na **fragmentação e na falta de padronização** dessas fontes.

O *pipeline* proposto visa resolver a "dor" da integração de dados, criando uma **fonte única de verdade** que combina:
1.  **Dados Estruturais (CVM):** Informações anuais e trimestrais sobre o balanço patrimonial e demonstrações de resultado.
2.  **Dados Comportamentais (YFinance/B3):** Cotações diárias, volumes de negociação e indicadores de mercado.

A construção deste *pipeline* é fundamental para viabilizar análises avançadas, como a avaliação de risco de crédito, a precificação de ativos e a criação de estratégias de investimento baseadas em fundamentos e comportamento de mercado.

## 2. Fontes de Dados e Características

A tabela a seguir resume as fontes de dados e suas principais características, destacando os desafios de integração.

| Fonte de Dados | Tipo de Informação | Granularidade | Desafio de Integração |
| :--- | :--- | :--- | :--- |
| **CVM (Dados Abertos)** | Demonstrações Financeiras Padronizadas (DFP) e Informações Trimestrais (ITR) [1]. | Anual e Trimestral. | **Padronização Semântica** (diferentes nomenclaturas para o mesmo indicador ao longo do tempo) e **Identificação** (uso do Código CVM). |
| **Yahoo Finance (YFinance)** | Cotações históricas (abertura, fechamento, máximo, mínimo, volume). | Diária. | **Identificação** (uso de *tickers* que podem variar) e **Qualidade** (dados podem conter falhas ou ajustes). |
| **B3 (Séries Históricas)** | Dados de negociação detalhados (opcionalmente *trade-a-trade* ou cotações diárias) [2]. | Diária ou *Trade-a-Trade*. | **Volume de Dados** (especialmente em granularidade alta) e **Padronização de Chaves** (uso de códigos de negociação). |

## 3. Arquitetura do Pipeline de Dados

O *pipeline* será construído seguindo o modelo ETL (Extract, Transform, Load), com foco na modularidade e na capacidade de reprocessamento.

### 3.1. Fase de Ingestão (Extract)

Esta fase é responsável pela coleta dos dados brutos de cada fonte.

| Fonte | Mecanismo de Ingestão | Frequência |
| :--- | :--- | :--- |
| **CVM** | Download de arquivos ZIP/CSV do Portal de Dados Abertos [1]. | Trimestral/Anual (ou sob demanda). |
| **YFinance** | Uso da biblioteca `yfinance` em Python para *scraping* de dados históricos via API. | Diária. |
| **B3** | Download de arquivos de séries históricas (seja via FTP ou portal web) [2]. | Diária. |

Os dados brutos são armazenados em uma **Camada *Landing*** (ou *Raw Data*), mantendo o formato original para garantir a rastreabilidade.

### 3.2. Fase de Tratamento e Transformação (Transform)

Esta é a fase mais crítica, onde a heterogeneidade é resolvida.

#### 3.2.1. Padronização de Chaves

O principal desafio é unificar as entidades. Será criado um **Mapeamento de Entidades** que correlaciona o **Código CVM** (fonte primária de identificação) com o **Ticker de Negociação** (usado em YFinance e B3).

#### 3.2.2. Tratamento de Dados Contábeis (CVM)

*   **Limpeza:** Remoção de registros duplicados e tratamento de valores nulos.
*   **Harmonização Semântica:** Criação de um dicionário de dados para padronizar as contas contábeis (e.g., mapear diferentes códigos de "Patrimônio Líquido" para uma única coluna `Patrimonio_Liquido`).
*   **Cálculo de Indicadores:** Derivação de métricas financeiras (ROE, Endividamento, Margens) a partir das contas primárias.

#### 3.2.3. Tratamento de Dados de Mercado (YFinance/B3)

*   **Limpeza:** Tratamento de *outliers* e ajuste de preços para eventos corporativos (splits, grupamentos).
*   **Cálculo de Risco:** Derivação de métricas de risco (Volatilidade, Retornos Diários) a partir das cotações.

### 3.3. Fase de Harmonização e Integração

Nesta fase, os dados tratados são combinados para formar o *dataset* analítico.

1.  **Criação da Tabela Fato:** Uma tabela central com granularidade diária.
2.  **Junção (Join):** Os dados de mercado (diários) são unidos aos dados contábeis (trimestrais/anuais) usando a **Data** e o **Identificador Padronizado** como chaves. Os dados contábeis são replicados para todos os dias dentro do período de validade da demonstração.
3.  **Camada Analítica (*Curated Data*):** O resultado é armazenado em uma camada otimizada para consultas, onde os dados de diferentes fontes coexistem de forma harmonizada.

## 4. Tecnologias Sugeridas

O *pipeline* pode ser implementado utilizando as seguintes tecnologias:

| Componente | Tecnologia Sugerida | Justificativa |
| :--- | :--- | :--- |
| **Orquestração** | Apache Airflow ou Prefect | Gerenciamento de dependências, agendamento e monitoramento do fluxo de trabalho. |
| **Processamento** | Python (Pandas, Dask) | Flexibilidade para manipulação de dados heterogêneos e cálculos complexos. |
| **Armazenamento** | PostgreSQL ou Data Lake (Parquet) | Armazenamento estruturado para a camada analítica e escalabilidade para dados brutos. |

## 5. Conclusão e Próximos Passos

A construção deste *pipeline* de integração de dados financeiros é um projeto de **engenharia de dados** que resolve o problema fundamental da heterogeneidade das fontes CVM, YFinance e B3. O produto final é um *dataset* de alta qualidade, essencial para qualquer iniciativa de análise de risco ou modelagem preditiva no mercado brasileiro.

Os próximos passos incluem a prototipagem da Fase de Ingestão e a validação do Mapeamento de Entidades, garantindo que a chave de unificação seja robusta e abrangente.

## Referências

[1] CVM. *Portal de Dados Abertos*. Disponível em: [https://dados.cvm.gov.br/](https://dados.cvm.gov.br/)
[2] B3. *Séries Históricas*. Disponível em: [https://www.b3.com.br/pt_br/market-data-e-indices/servicos-de-dados/market-data/historico/mercado-a-vista/series-historicas/](https://www.b3.com.br/pt_br/market-data-e-indices/servicos-de-dados/market-data/historico/mercado-a-vista/series-historicas/)
