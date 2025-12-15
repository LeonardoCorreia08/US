# Projeto de Engenharia de Dados: Construção de um Pipeline de Integração Financeira

## Resumo Executivo

Este projeto detalha a construção de um *pipeline* de **Engenharia de Dados** desenhado para integrar e harmonizar dados financeiros de fontes primárias brasileiras: **CVM**, **Yahoo Finance (YFinance)** e uma base histórica da **B3**. O escopo temporal do projeto abrange um período extenso, com dados de mercado desde 2000 e dados contábeis desde 2010, complementados por uma base histórica da B3 de 1994 a 2021. O principal resultado é a criação de uma **Base Financeira Integrada** estruturada em quatro camadas lógicas (Estatística, Contábil, de Mercado e Temporal), que resolve a heterogeneidade de granularidade e semântica. O processo inclui uma rigorosa fase de *profiling* e qualidade, que resultou em um *dataset* com alta integridade, pronto para análises avançadas e modelos de *Machine Learning*.

## 1. Introdução e Justificativa

A análise financeira robusta exige a combinação de informações de diferentes naturezas. A dificuldade reside na **fragmentação e na disparidade temporal e estrutural** dos dados. O *pipeline* proposto visa criar uma **fonte única de verdade** que combina:
*   **Fundamentos:** Dados contábeis (CVM) que refletem a saúde estrutural das companhias.
*   **Comportamento:** Cotações e retornos (YFinance/B3) que capturam a dinâmica de mercado.

A construção deste *pipeline* é justificada pela necessidade de um *dataset* limpo e harmonizado que suporte análises complexas, como a correlação entre fundamentos (ROE, Endividamento) e risco de mercado (Volatilidade), sem a necessidade de manipulação manual e propensa a erros.

## 2. Cobertura e Escopo dos Dados

O projeto abrange uma cobertura histórica significativa, essencial para o treinamento de modelos preditivos e análises de longo prazo.

| Fonte de Dados | Tipo de Informação | Cobertura Temporal | Granularidade |
| :--- | :--- | :--- | :--- |
| **CVM (Dados Abertos)** | Demonstrações Financeiras Padronizadas (DFP) e ITR. | Desde 2010 | Anual e Trimestral |
| **Yahoo Finance (YFinance)** | Cotações históricas (preços, volume). | Desde 2000 | Diária |
| **B3 (Base SQL)** | Dados de negociação históricos. | 1994 a 2021 | Diária ou *Trade-a-Trade* |

## 3. Fase de Ingestão e Qualidade de Dados

A fase inicial de ingestão coleta os dados brutos, que são imediatamente submetidos a um processo de **profiling** para garantir a qualidade antes da transformação.

> "Antes da integração, foi realizado um profiling completo das bases financeiras, permitindo identificar diferenças de granularidade, chaves heterogêneas e arquivos de natureza estatística, evitando *merges* indevidos."

O resultado do *profiling* inicial confirmou a alta qualidade dos dados brutos e a correta separação dos arquivos:
*   **Integridade Estrutural:** Foram identificadas **199 combinações arquivo × coluna**, indicando uma estrutura de dados bem separada e organizada.
*   **Tipagem:** A tipagem dos dados é majoritariamente correta (`float64`, `int64`, `object`), facilitando o processamento.
*   **Completude:** O *dataset* apresenta **zero nulos relevantes**, um fator excelente para a confiabilidade das análises e para o treinamento de modelos de *Machine Learning*.

## 4. Arquitetura da Base Financeira Integrada (4 Camadas)

A Base Financeira Integrada é o produto final do *pipeline* e é organizada em quatro camadas lógicas, cada uma com um propósito analítico distinto. Esta separação garante que os dados sejam usados de forma apropriada, respeitando suas granularidades e funções.

| Camada | Arquivos de Exemplo | Estrutura Lógica | Função Analítica |
| :--- | :--- | :--- | :--- |
| **Base Estatística** | `base_estatistica_cvm_macro.csv` | Colunas numéricas puras, chave `referencia`. | **Benchmark Macroeconômico.** Serve como régua de comparação. **Não deve ser usada em *merge* linha a linha** com dados de empresas. |
| **Base Contábil** | `base_contabil_*` | Chave: `Empresa / ativo, Ano`. | **Nível Micro (Empresa/Ano).** Contém Receita, Lucro, ROE, Endividamento. Separação correta de granularidade em relação à Base Estatística. |
| **Base de Mercado** | `base_mercado_retornos_diarios.csv` | Formato *Wide* (coluna por ativo), métricas homogêneas. | **Risco e Correlação.** Ideal para calcular Risco Sistêmico, Beta e PCA/Clustering. **Não misturar diretamente com CVM** (dados contábeis). |
| **Base Temporal** | `base_temporal_*` | Chave: `Data + Ativo`. | **Granularidade Mais Fina.** Contém Preço, Retorno, Indicadores Técnicos (RSI, SMA, Volatilidade). Base ideal para *Trading*, Modelos Preditivos e *Forecast*. |

## 5. Fase de Harmonização e Transformação

A fase de transformação foca em criar as chaves de ligação entre as quatro bases, permitindo que o analista ou o modelo de ML combine as informações conforme a necessidade.

### 5.1. Mapeamento de Chaves e Granularidade

O principal desafio é a unificação de granularidades:
*   **Contábil (Trimestral/Anual) + Mercado/Temporal (Diário):** A integração é feita replicando os dados contábeis (nível micro) para todos os dias dentro do período de validade da demonstração, usando a chave **Ativo + Data**.
*   **Estatística (Macro):** Esta base é mantida separada, servindo como uma **tabela de referência** para contextualização, e não para junção direta com as tabelas de empresas.

### 5.2. Produtos Finais do Pipeline

O *pipeline* gera diversos arquivos de saída (CSV) que atendem a diferentes necessidades analíticas, conforme detalhado na análise de uso:

| Objetivo Analítico | Arquivos de Saída (Exemplos) | Uso Prático |
| :--- | :--- | :--- |
| **Seleção de Carteira** | `ranking_performance.csv`, `clusters_acoes.csv` | Filtrar ativos por Retorno/Risco, montar carteiras diversificadas por cluster. |
| **Timing e Trading** | `indicadores_tecnicos_completo.csv`, `backtest_estrategia_mm.csv` | Alimentar robôs de *trade*, validar estratégias de média móvel, identificar pontos de entrada. |
| **Segurança e Risco** | `2_risco_atr.csv`, `anomalias_volume.csv` | Calcular *Stop Loss* (via ATR), investigar eventos de *Insider/News* (via anomalias de volume). |

## 6. Conclusão

O projeto de *pipeline* demonstra uma abordagem de **engenharia de dados** que transforma a complexidade da integração de dados financeiros heterogêneos (CVM, YFinance, B3) em um ativo estruturado e de alta qualidade. A arquitetura de quatro camadas e a confirmação de integridade dos dados garantem que a **Base Financeira Integrada** seja uma plataforma sólida para qualquer análise de risco, *valuation* ou estratégia de investimento, superando os desafios de granularidade e semântica.

## Referências

[1] CVM. *Portal de Dados Abertos*. Disponível em: [https://dados.cvm.gov.br/](https://dados.cvm.gov.br/)
[2] B3. *Séries Históricas*. Disponível em: [https://www.b3.com.br/pt_br/market-data-e-indices/servicos-de-dados/market-data/historico/mercado-a-vista/series-historicas/](https://www.b3.com.br/pt_br/market-data-e-indices/servicos-de-dados/market-data/historico/mercado-a-vista/series-historicas/)
