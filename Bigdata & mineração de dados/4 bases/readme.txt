O QUE O RELATÓRIO CONFIRMA (VISÃO GERAL)

Você tem 199 combinações arquivo × coluna, o que indica:

✔️ Bases bem separadas

✔️ Nenhum CSV vazio

✔️ Tipagem majoritariamente correta (float64, int64, object)

✔️ Zero nulos relevantes (excelente para análise e ML)

ANÁLISE POR CAMADA (AS 4 BASES)
BASE ESTATÍSTICA

base_estatistica_cvm_macro.csv

O que contém

Estatísticas agregadas do mercado brasileiro

Colunas numéricas puras

Linha de referência (count, mean, std, etc.)

Estrutura detectada
Tipo	Colunas
Identificador	referencia
Métricas	Receita_Liquida, Lucro_Liquido, Margem_Liquida, ROE, ROA
Interpretação correta

✔️ Benchmark macroeconômico
✔️ Não se relaciona diretamente por chave com empresas
✔️ Serve como régua de comparação

Nunca deve ser usada em merge linha a linha

BASE CONTÁBIL

base_contabil_*

O que contém:

Dados anuais por empresa

Métricas contábeis clássicas

Estrutura lógica
Tipo	Colunas
Chave	Empresa / ativo, Ano
Financeiro	Receita, Lucro, Ativo, Passivo
Indicadores	ROE, ROA, Margem, Endividamento
Observação importante

Algumas métricas aparecem também na base estatística, mas:

Aqui → nível micro (empresa/ano)

Lá → nível macro (mercado agregado)

✔Separação correta de granularidade

BASE DE MERCADO

base_mercado_*

Exemplos detectados

base_mercado_fundamentalistas.csv

base_mercado_ranking.csv

base_mercado_retornos_diarios.csv

Evidência clara no relatório
base_mercado_retornos_diarios.csv
AMZN, HD, ^GSPC, ^DJI, ^IXIC → float64

Interpretação

Formato wide (uma coluna por ativo)
Métricas homogêneas (retornos diários)

Classificação
Tipo	Conteúdo
Ativos	AAPL, MSFT, AMZN
Índices	^GSPC, ^DJI, ^IXIC
Métrica	Retorno percentual

Excelente para:

Correlação

Risco sistêmico

Beta

PCA / Clustering

Não misturar diretamente com CVM


BASE TEMPORAL

base_temporal_*

O que ela representa

Séries temporais

Dados ordenados por tempo

Preço, retorno, indicadores técnicos

Estrutura implícita
Chave	Métrica
Data	Preço
Ativo	Retorno
Data + Ativo	RSI, SMA, Volatilidade

✔️ Granularidade mais fina do projeto
✔️ Base ideal para:

Trading

Modelos preditivos

Forecast