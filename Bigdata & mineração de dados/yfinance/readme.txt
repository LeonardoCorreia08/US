Estes arquivos cobrem um espectro completo de análise de mercado, focando especificamente no mercado americano (S&P 500, Nasdaq) e grandes empresas de tecnologia (Big Techs).

Aqui está o "manual de uso" para cada um, dividido por objetivo:

1.Para Avaliação de Empresas (Valuation)
dados_fundamentalistas.csv
O que é: O "boletim escolar" das empresas. Traz métricas de qualidade e preço.

Colunas Chave: PE (Preço/Lucro), ROE (Retorno sobre Patrimônio), Dividend_Yield, Profit_Margin.

Para que serve: Decidir se uma ação está barata ou cara em relação ao que ela entrega.

Exemplo Prático:

Comparar o PE da NVDA (Nvidia) com o da MSFT (Microsoft) para ver quem está mais "esticada".

Filtrar empresas com Debt_to_Equity (Dívida/Patrimônio) baixo para evitar empresas quebradas.

2. Para Análise de Risco e Gestão de Portfólio
estatisticas_retornos.csv
O que é: O mapa de perigos. Analisa o comportamento percentual diário dos ativos.

Colunas Chave: Desvio_Padrão_Diário (Volatilidade), VaR_95% (Value at Risk), Skewness (Assimetria).

Para que serve: Saber o quanto você pode perder num dia ruim.

Exemplo Prático:

Olhar o VaR 95%: Se o VaR da Apple for -3.5%, significa que existe 5% de chance dela cair mais que 3.5% amanhã.

Comparar a volatilidade do Bitcoin (se estivesse na lista) com a do Ouro (ou ações conservadoras como JNJ).

dados_retornos_diarios.csv
O que é: A variação percentual dia a dia.

Para que serve: Calcular correlações (quais ativos sobem juntos) e Beta.

Exemplo Prático: Usar para calcular o Beta da sua carteira em relação ao S&P 500 (^GSPC).

3. Para Análise Técnica e Trading
indicadores_tecnicos.csv
O que é: Sinais prontos de compra e venda calculados sobre o preço.

Colunas Chave: SMA50/SMA200 (Médias Móveis), RSI (Índice de Força Relativa).

Para que serve: Identificar tendências.

Exemplo Prático:

Cruzamento Dourado: Se a SMA50 cruzar para cima da SMA200, é um sinal clássico de tendência de alta de longo prazo.

RSI: Se o RSI da META cair abaixo de 30, pode indicar que caiu demais ("sobrevendido") e pode repicar.

dados_precos_fechamento.csv
O que é: A cotação bruta histórica (desde 2000 até hoje).

Para que serve: A base de tudo. Serve para desenhar os gráficos de linha ou candlestick.

Exemplo Prático: Ver a queda de 2008 ou a recuperação pós-pandemia no gráfico.

4. Estatísticas Descritivas (Curiosidade/Contexto)
estatisticas_precos.csv
O que é: Resumo dos preços nominais (Média, Máximo, Mínimo em Dólares).

Atenção: Estatística de preço (em vez de retorno) pode enganar em prazos longos, pois uma ação que custava $1 em 2000 e custa $200 hoje terá uma média que não diz muito sobre o momento atual.

Para que serve: Ter noção da amplitude histórica.

Exemplo Prático: Saber que a mínima histórica da NVDA foi $0.05 (ajustado) e a máxima $207, mostrando a explosão exponencial da empresa.

Resumo: Por onde começar?
Investidor Fundamentalista: Comece pelo dados_fundamentalistas.csv para escolher as melhores empresas.

Gestor de Risco: Use o estatisticas_retornos.csv para não correr riscos desnecessários.

Trader/Grafista: Vá direto para o indicadores_tecnicos.csv e dados_precos_fechamento.csv.