# Atividade Hello World (4/6)

**Conteúdo principal**

**Condições de conclusão:**
* Reproduza os passos do Hello World;
* Print da tela do Wokwi rodando o Hello World;
* Análise do código e documentação em um breve relatório sobre as observações encontradas;

---

| Alvos Suportados | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- | --------- | ----- |

# Exemplo Hello World

Inicia uma tarefa do FreeRTOS para imprimir "Hello World".

*(Consulte o arquivo README.md no diretório 'examples' superior para obter mais informações sobre os exemplos.)*

## Como usar o exemplo

Siga as instruções detalhadas fornecidas especificamente para este exemplo.

Selecione as instruções dependendo do chip Espressif instalado na sua placa de desenvolvimento:

* [Guia de Introdução ao ESP32](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
* [Guia de Introdução ao ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)

## Conteúdo da pasta do exemplo

O projeto **hello_world** contém um arquivo fonte na linguagem C [hello_world_main.c](main/hello_world_main.c). O arquivo está localizado na pasta [main](main).

Os projetos do ESP-IDF são construídos usando o CMake. A configuração de compilação do projeto está contida nos arquivos `CMakeLists.txt`, que fornecem um conjunto de diretrizes e instruções descrevendo os arquivos fonte e os alvos do projeto (executável, biblioteca ou ambos).

Abaixo está uma breve explicação dos arquivos restantes na pasta do projeto:

```text
├── CMakeLists.txt
├── pytest_hello_world.py     Script Python usado para testes automatizados
├── main
│   ├── CMakeLists.txt
│   └── hello_world_main.c
└── README.md                 Este é o arquivo que você está lendo atualmente