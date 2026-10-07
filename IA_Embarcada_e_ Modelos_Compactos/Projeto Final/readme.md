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

