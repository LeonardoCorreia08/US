# Atividade Hello World (4/6)

**Conteúdo principal**

**Condições de conclusão:**
* Reproduza os passos do Hello World;
* Print da tela do Wokwi rodando o Hello World;
* Análise do código e documentação em um breve relatório sobre as observações encontradas;

---

| Alvos Suportados | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- | --------- | ----- |

# Relatório Prático: Hello World com Inteligência Artificial no ESP32-S3

Este documento registra o desenvolvimento e a conclusão da atividade **Hello World (4/6)**, abordando a implementação de uma rede neural embarcada (TinyML) no microcontrolador **ESP32-S3** através do framework oficial **ESP-IDF** e do componente **TensorFlow Lite Micro**.

---

## 1. O que foi feito nesta atividade?

Em sistemas embarcados tradicionais, o clássico "Hello World" costuma ser apenas piscar um LED ou imprimir um texto na tela. Aqui, demos um passo além: implementamos uma rede neural real embarcada para calcular previsões matemáticas em tempo real.

O projeto foi validado em ambiente simulado e containerizado, cumprindo todos os requisitos:
- Compilação do firmware usando o ecossistema **ESP-IDF v6.1**;
- Conversão de um modelo de Inteligência Artificial (\`.tflite\`) para código que o microcontrolador entende;
- Execução contínua da inferência com gerenciamento de tarefas pelo **FreeRTOS**;
- Validação visual e de logs no simulador **Wokwi**.

---

## 2. Como tudo isso funciona? (
Para quem está começando, pode parecer estranho rodar uma IA em um chip tão pequeno. O fluxo funciona em 4 etapas diretas:

```
[ Modelo Treinado .tflite ]
            │
            ▼ (conversão via Python)
[ Arquivo C/C++ hello_world_int8.h ]  <-- Array de bytes na memória flash
            │
            ▼ (alimenta o motor de IA)
[ Interpretador TFLite Micro (.cc) ]  <-- Faz os cálculos matemáticos
            │
            ▼ (chama a cada 500 ms)
[ Código Principal C (hello_world_main.c) ]  <-- Mostra o resultado no terminal
```

1. **O Modelo (\`hello_world_int8.tflite\`):** É a rede neural treinada. Como o microcontrolador não tem um disco rígido para "abrir" um arquivo, nós transformamos esse arquivo em uma lista de números hexadecimais em C (\`hello_world_int8.h\`) para ele ficar gravado diretamente na memória flash do chip.
2. **A "Ponte" C e C++ (\`tflite_runner_v1.cc\` e \`.h\`):** O TensorFlow Lite é escrito em C++, mas o ponto de entrada padrão do ESP-IDF usa C puro. Criamos um adaptador (*wrapper*) para que o nosso código em C consiga pedir previsões para a IA sem dar conflitos entre linguagens.
3. **Quantização INT8:** Redes neurais no computador usam números quebrados pesados (\`float32\`). Para rodar no ESP32 rápido e consumindo pouca memória, o modelo usa números inteiros de 8 bits (\`int8\`). O nosso código converte o valor antes de entregar para a IA e depois reconverte a resposta para um número legível.
4. **O Loop Principal (\`hello_world_main.c\`):** O programa principal inicializa a IA uma única vez e, usando uma tarefa do FreeRTOS com \`vTaskDelay\`, faz perguntas contínuas para o modelo a cada 500 milissegundos.

---

## 3. Estrutura dos Arquivos

Dentro da pasta do projeto, a organização dos arquivos ficou assim:

\`\`\`text
├── CMakeLists.txt              # Configuração global do projeto ESP-IDF
├── README.md                   # Este relatório descritivo
└── main/
    ├── CMakeLists.txt          # Declaração dos arquivos e dependências
    ├── hello_world_main.c      # Loop principal em C (FreeRTOS + prints)
    ├── hello_world_int8.h      # Os bytes do modelo de IA embarcado
    ├── tflite_runner.h         # Declaração das funções da ponte (Header C)
    └── tflite_runner_v1.cc     # Implementação do motor de inferência (C++)
\`\`\`

---

## 4. Registro de Execução e Evidências

### Log de Inicialização e Inferência Real

Durante a execução no terminal serial, o bootloader carrega os componentes e entrega o controle para a função principal, iniciando o ciclo de predição:

\`\`\`text
I (209) main_task: Calling app_main()
Inicializando modelo TFLite...
Modelo carregado com sucesso!
x: 0.00 | y (inferencia): 0.727567
x: 0.20 | y (inferencia): 0.672208
x: 0.40 | y (inferencia): 0.608942
x: 0.60 | y (inferencia): 0.537767
x: 0.80 | y (inferencia): 0.434958
x: 1.00 | y (inferencia): 0.308425
x: 1.20 | y (inferencia): 0.221433
x: 1.40 | y (inferencia): 0.134442
x: 1.60 | y (inferencia): 0.031633
x: 1.80 | y (inferencia): -0.063267
x: 2.00 | y (inferencia): -0.189800
x: 2.20 | y (inferencia): -0.340058
x: 2.40 | y (inferencia): -0.482408
x: 2.60 | y (inferencia): -0.703842
\`\`\`

### Comprovação Visual no Simulador (Wokwi)

> **Observação para entrega:** Insira aqui a captura de tela (print) da janela completa do Wokwi mostrando a placa simulada e o terminal serial rodando os valores acima.
> 
> *(Espaço reservado para o Print da Tela do Wokwi)*

---

## 5. Análise Técnica e Aprendizados do Processo

Durante a montagem e compilação do exercício, alguns desafios técnicos reais de desenvolvimento de sistemas embarcados foram encontrados e solucionados:

1. **Gestão de Dependências no CMake:** O compilador precisa saber explicitamente onde procurar os cabeçalhos (\`INCLUDE_DIRS "."\`). Sem isso, o arquivo \`hello_world_int8.h\` não é encontrado durante a montagem do binário.
2. **Integração C e C++ (Linker):** Como a biblioteca do TFLite Micro é estruturada com classes e objetos C++, foi indispensável envolver as funções exportadas em blocos \`extern "C"\`. Isso impediu erros de *undefined reference* causados pela alteração de nomes (*name mangling*) feita pelo g++.
3. **Alinhamento de Memória:** O interpretador do TensorFlow Micro exige que o buffer de memória (\`tensor_arena\`) e os bytes do modelo estejam estritamente alinhados a múltiplos de 16 bytes (\`alignas(16)\`), evitando falhas de hardware no acesso aos registradores de memória do ESP32-S3.
4. **Ciclo de Vida no FreeRTOS:** Na primeira tentativa de execução, o chip processava uma única inferência e encerrava a execução (\`Returned from app_main()\`). A adição de um laço infinito temporizado com \`vTaskDelay(pdMS_TO_TICKS(500))\` garantiu que o sistema permaneça ativo sem travar a CPU ou acionar o Watchdog Timer.`;
