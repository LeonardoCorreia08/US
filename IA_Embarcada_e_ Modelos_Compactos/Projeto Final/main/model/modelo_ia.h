#ifndef MODELO_IA_H
#define MODELO_IA_H

// Executa a inferência da Rede Neural em C puro
// Entradas: Dia, pH, Temperatura, Turbidez, CO2
// Saídas: Vetor com 4 posições [DQO, Fosforo, Nitrogenio, Enxofre]
void prever_bioprocesso(float dia, float ph, float temp, float turbidez, float co2, float* resultados_saida);

#endif // MODELO_IA_H