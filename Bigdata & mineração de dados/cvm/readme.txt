Aqui está o que cada um contém e como você pode usá-los:

1.cvm_dados_consolidados.csv (A Base Bruta)
Este é o arquivo "raiz". Ele contém os dados contábeis puros extraídos dos balanços das empresas, sem cálculos adicionais.

O que contém: Dados anuais de Receita, Resultado Bruto, Ativo Total, Patrimônio Líquido e Passivo para empresas como Petrobras, Ambev, etc.

Para quem serve: Para quem quer criar seus próprios indicadores do zero ou precisa validar os números originais.

Insight possível:

Ver a evolução do "tamanho" da empresa (Ativo Total) ao longo dos anos.

Calcular métricas de solvência (quanto ela tem de bens vs. dívidas).

Nota: Notei que em várias linhas a coluna Lucro_Liquido está zerada. Isso sugere que você deve olhar com cuidado para a coluna Resultado_Antes_IR ou verificar se o lucro foi lançado em outra conta contábil na origem.

2.cvm_indicadores_financeiros.csv (A Análise Pronta)
Este é o arquivo mais valioso para um analista. Ele pega a base bruta anterior e adiciona colunas calculadas de inteligência financeira.

O que contém: Tudo o que o arquivo anterior tem, MAIS indicadores clássicos:

ROE (Retorno sobre Patrimônio): Quanto a empresa lucra para cada real dos sócios.

ROA (Retorno sobre Ativo): Eficiência da empresa em gerar lucro com o que ela tem.

Margem Líquida: Eficiência operacional.

Endividamento: O nível de alavancagem.

Crescimento de Receita: A velocidade de expansão.

Para quem serve: Para economizar tempo. Se você quer fazer um gráfico da rentabilidade da Ambev x Itaú, use este arquivo direto.

Insight possível:

Identificar empresas que estão crescendo vendas (Crescimento_Receita alto) mas perdendo eficiência (Margem caindo).

3.cvm_medias_por_empresa.csv (O Ranking/Benchmarking)
Este arquivo resume a história de cada empresa em uma única linha, tirando a média de todos os anos disponíveis.

O que contém: Uma linha por empresa (Ambev, B3, Bradesco, Itaú, Magalu, etc.) com a média de seus indicadores.

Para quem serve: Para comparar "quem é melhor que quem" estruturalmente, ignorando um ano ruim específico.

Insight possível:

Responder perguntas como: "Historicamente, quem é mais rentável (maior ROE médio): Bancos ou Varejo (Magalu)?"

Criar um gráfico de barras comparando o endividamento médio de cada setor.

4.cvm_estatisticas_descritivas.csv (A Visão Macro)
Este arquivo não olha para empresas individuais, mas para o conjunto de dados inteiro. É o resultado do comando .describe() do Python.

O que contém: Média geral, desvio padrão (risco), mínimo, máximo e quartis (25%, 50%, 75%) de todas as métricas.

Para quem serve: Para entender o "padrão" do mercado e detectar anomalias (outliers).

Insight possível:

Definir o que é "bom": Se o ROE médio do mercado é 15%, uma empresa com 20% é excelente. Sem esse arquivo, você não teria a régua para medir.

Checagem de erros: Se o valor min da Receita for negativo, você sabe que tem um erro na base (pois não existe receita negativa).


Resumo Prático: Qual arquivo abrir?

Quer fazer gráficos de evolução ano a ano? Use cvm_indicadores_financeiros.csv.

Quer saber qual a melhor empresa do grupo? Use cvm_medias_por_empresa.csv.

Quer calcular indicadores exóticos que ninguém usa? Use a base bruta cvm_dados_consolidados.csv.