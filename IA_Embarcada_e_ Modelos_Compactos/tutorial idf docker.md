# ESP-IDF 6.1 com Docker + VS Code

---

# 1. Visão geral da configuração

A estrutura utilizada será:

```text
Windows
│
├── Docker Desktop
│
├── C:\docker\
│   └── projeto\
│       └── arquivos do projeto ESP-IDF
│
└── VS Code
    │
    └── Dev Containers
            │
            └── Container ESP-IDF 6.1
                    │
                    └── /workspace
```

O diretório:

```text
C:\docker\projeto
```

será compartilhado com o container como:

```text
/workspace
```

Assim:

```text
Windows                         Docker
────────────────────────────────────────
C:\docker\projeto      <──>    /workspace
```

Qualquer arquivo criado no `/workspace` também estará disponível no Windows.

---

# 2. Pré-requisitos

Antes de começar, instale:


* Docker Desktop ou docker (linux)
* Visual Studio Code * Extensão **Dev Containers** do VS Code

Também é recomendado utilizar:

* Git
* PowerShell ou CMD

---

# 3. Instalar o Docker Desktop

Baixe e instale o Docker Desktop para Windows.

Durante a instalação, mantenha a configuração padrão, principalmente a utilização do **WSL 2**, caso seja oferecida.

Depois da instalação, abra o:

**Docker Desktop**

Aguarde até que o Docker esteja funcionando.

---

# 3. Criar a pasta de trabalho do Docker

para simplificar:

```text
C:\docker\projeto
```

Essa será a pasta compartilhada entre o Windows e o container.

Ela poderá ser criada manualmente:

```text
C:\
└── docker
    └── projeto
```

ou será criada automaticamente pelo script da próxima etapa.

---

# 4. Criar o arquivo `idf_install_docker.bat`

Crie um arquivo chamado:

```text
idf_install_docker.bat
```

Por exemplo:

```text
C:\docker\idf_install_docker.bat
```

Coloque dentro dele:

```bat
@echo off

:: ==========================================
:: ESP-IDF Docker
:: ==========================================

:: Cria a pasta C:\docker\projeto
:: caso ela ainda nao exista
if not exist "C:\docker\projeto" (
    mkdir "C:\docker\projeto"
    echo Pasta "C:\docker\projeto" criada com sucesso.
)

:: Forca a exclusao do container antigo
:: para evitar erro de nome duplicado
echo.
echo Removendo container antigo (se existir)...
docker rm -f meu-projeto-esp >nul 2>&1

:: Inicia o container novo
:: mapeando C:\docker\projeto para /workspace
echo.
echo Criando e iniciando o container meu-projeto-esp...

docker run -d --name meu-projeto-esp -v "C:\docker\projeto:/workspace" -w /workspace espressif/idf:release-v6.1 tail -f /dev/null

echo.
echo ==========================================
echo Pronto! O container esta verde e rodando.
echo Pode colocar seus arquivos na pasta C:\docker\projeto,
echo abrir o VS Code e anexar.
echo ==========================================

pause
```

---

# 5. Executar o `idf_install_docker.bat`

Clique duas vezes no arquivo:

```text
idf_install_docker.bat
```

O script executará algumas etapas.

## 5.1. Criar a pasta

Caso não exista:

```text
C:\docker\projeto
```

ela será criada.

---

## 5.2. Remover container antigo

O script executa:

```bash
docker rm -f meu-projeto-esp
```

Isso evita o erro:

```text
Conflict. The container name is already in use
```

Caso o container ainda não exista, nada acontecerá.

---

# 6. Criar o container ESP-IDF

O comando principal utilizado pelo script é:

```bash
docker run -d --name meu-projeto-esp -v "C:\docker\projeto:/workspace" -w /workspace espressif/idf:release-v6.1 tail -f /dev/null
```

Cada parte possui uma função.

### `--name`

Define o nome do container:

```text
meu-projeto-esp
```

### `-v`

Cria o compartilhamento:

```text
C:\docker\projeto
```

com:

```text
/workspace
```

### `-w`

Define:

```text
/workspace
```

como diretório de trabalho.

### Imagem

Utilizamos:

```text
espressif/idf:release-v6.1
```

que fornece o ambiente ESP-IDF 6.1.

### `tail -f /dev/null`

Mantém o container rodando continuamente.

---

# 7. Verificar se o container está funcionando

Abra o PowerShell e execute:

```powershell
docker ps
```

Você deverá encontrar algo semelhante a:

```text
CONTAINER ID   IMAGE                         STATUS         NAMES
xxxxxxxxxxxx   espressif/idf:release-v6.1   Up ...         meu-projeto-esp
```

O ponto importante é:

```text
STATUS: Up
```

Isso significa que o container está em execução.

---

# 8. Instalar a extensão Dev Containers no VS Code

Abra o:

**Visual Studio Code**

Abra:

```text
Extensions
```

ou pressione:

```text
Ctrl + Shift + X
```

Pesquise por:

```text
Dev Containers
```

Instale a extensão oficial da Microsoft:

```text
Dev Containers
```

Ela permite trabalhar diretamente dentro de containers Docker.

---

# 9. Criar um novo projeto

Dentro de:

```text
C:\docker\projeto
```

crie uma pasta para o projeto.

Por exemplo:

```text
C:\docker\projeto\Project_aula_2_IOT
```

A estrutura inicialmente poderá ser:

```text
C:\docker\projeto\
└── Project_aula_2_IOT\
```

---

# 10. Criar a pasta `.devcontainer`

Dentro do projeto:

```text
Project_aula_2_IOT
```

crie:

```text
.devcontainer
```

A estrutura ficará:

```text
Project_aula_2_IOT
│
└── .devcontainer
```

> A pasta começa com ponto porque `.devcontainer` é uma pasta de configuração do ambiente de desenvolvimento.

---

# 11. Criar o `devcontainer.json`

Dentro da pasta:

```text
.devcontainer
```

crie:

```text
devcontainer.json
```

Estrutura:

```text
Project_aula_2_IOT
│
└── .devcontainer
    └── devcontainer.json
```

Coloque:

```json
{
    "name": "Ambiente ESP-IDF",
    "image": "espressif/idf:release-v6.1",
    "workspaceMount": "source=${localWorkspaceFolder},target=/workspace,type=bind",
    "workspaceFolder": "/workspace"
}
```

---

# 12. Entendendo o `devcontainer.json`

## `name`

```json
"name": "Ambiente ESP-IDF"
```

Nome exibido pelo VS Code para o ambiente.

---

## `image`

```json
"image": "espressif/idf:release-v6.1"
```

Define a imagem Docker utilizada pelo Dev Container.

---

## `workspaceMount`

```json
"workspaceMount": "source=${localWorkspaceFolder},target=/workspace,type=bind"
```

Faz o compartilhamento da pasta atual do projeto com:

```text
/workspace
```

---

## `workspaceFolder`

```json
"workspaceFolder": "/workspace"
```

Define:

```text
/workspace
```

como diretório principal dentro do container.

---

# 13. Abrir o projeto no VS Code

No VS Code:

```text
Arquivo
    ↓
Abrir Pasta...
```

Ou utilize:

```text
Ctrl + K
Ctrl + O
```

Selecione:

```text
C:\docker\projeto\Project_aula_2_IOT
```

---

# 14. Abrir o projeto dentro do container

Depois que o projeto estiver aberto, pressione:

```text
Ctrl + Shift + P
```

Digite:

```text
Dev Containers
```

Procure a opção:

```text
Dev Containers: Reopen in Container
```

ou, dependendo da versão:

```text
Dev Containers: Reopen Folder in Container
```

Selecione a opção.

---

# 16. Aguardar a inicialização

O VS Code irá:

1. Ler o `.devcontainer/devcontainer.json`
2. Localizar a imagem ESP-IDF
3. Criar/iniciar o ambiente
4. Montar o projeto
5. Abrir o VS Code dentro do container

Na parte inferior/esquerda do VS Code deverá aparecer algo indicando que você está conectado ao container.

Por exemplo:

```text
Dev Container: Ambiente ESP-IDF
```

---

# 17. Abrir `/workspace`

Caso o VS Code não abra automaticamente a pasta correta, utilize:

```text
Arquivo
    ↓
Abrir Pasta...
```

Na barra superior que aparecerá, digite exatamente:

```text
/workspace
```

Depois pressione:

```text
Enter
```

ou confirme no botão correspondente.

O VS Code deverá mostrar o conteúdo do projeto dentro de:

```text
/workspace
```

---

# 18. Abrir o terminal do container

No VS Code, abra:

```text
Terminal
    ↓
Novo Terminal
```

Atalho:

```text
Ctrl + `
```

> O caractere é a crase/backtick, normalmente localizado na mesma tecla do `~`.

Também é possível utilizar:

```text
Ctrl + Shift + `
```

dependendo da configuração do VS Code.

O terminal deverá estar dentro do container.

---


# 20. Carregar o ambiente ESP-IDF

No terminal do container execute:

```bash
. /opt/esp/idf/export.sh
```

Esse comando carrega as variáveis de ambiente necessárias para utilizar o ESP-IDF.

Depois disso, você poderá verificar:

```bash
idf.py --version
```

O resultado deverá indicar a versão do ESP-IDF instalada na imagem.

---

# 21. Limpar o projeto

Antes de uma compilação limpa, execute:

```bash
idf.py fullclean
```

Esse comando realiza uma limpeza completa dos arquivos de compilação gerados pelo ESP-IDF.

---

# 22. Remover manualmente a pasta `build`

Também podemos remover a pasta:

```bash
rm -rf build
```

Execute:

```bash
rm -rf build
```

A pasta `build` não deverá mais aparecer.

> Normalmente `idf.py fullclean` já trata a limpeza necessária, mas remover `build` manualmente pode ser útil quando você quer garantir uma compilação completamente nova.

---

# 23. Compilar o projeto

Agora execute:

```bash
idf.py build
```

O ESP-IDF começará a compilar o projeto.

Durante o processo aparecerão mensagens semelhantes a:

```text
-- Building ESP-IDF components for target esp32
...
[100%] Built target ...
```

Ao final, o projeto deverá ser compilado com sucesso.

---

# 24. Fluxo completo

Depois que tudo estiver configurado, o fluxo normal será:

```text
1. Abrir Docker Desktop
        ↓
2. Container ESP-IDF funcionando
        ↓
3. Abrir projeto no VS Code
        ↓
4. Reopen in Container
        ↓
5. Abrir Terminal
        ↓
6. Carregar ESP-IDF
        ↓
7. Limpar projeto
        ↓
8. Compilar
```

Comandos:

```bash
. /opt/esp/idf/export.sh

idf.py fullclean

rm -rf build

idf.py build
```

---

# 25. Estrutura final do projeto

Depois da configuração, uma estrutura típica será:

```text
C:\docker\projeto\
│
└── Project_aula_2_IOT\
    │
    ├── .devcontainer\
    │   └── devcontainer.json
    │
    ├── main\
    │   ├── main.c
    │   └── CMakeLists.txt
    │
    ├── CMakeLists.txt
    ├── sdkconfig
    └── ...
```

Depois da compilação:

```text
C:\docker\projeto\
│
└── Project_aula_2_IOT\
```
# 26. Adicionar a extensão do Wokwi

## 26.1. Criar o wokwi.toml

Na raiz do projeto, crie:

`wokwi.toml`

Exemplo:

```toml
[wokwi]
version = 1
elf = "build/Project_aula_2_IOT.elf"
firmware = "build/flasher_args.json"

```

> **Importante:** o nome do arquivo `.elf` deve ser exatamente o mesmo gerado durante a compilação do projeto.

---

## 26.2. Executar a simulação

Depois da compilação do projeto, utilize a **extensão Wokwi** instalada no VS Code.

O projeto deverá possuir a configuração necessária do Wokwi, incluindo:

- `wokwi.toml`
- arquivo `.elf` gerado na pasta `build/`
- `flasher_args.json` gerado na pasta `build/`
- arquivo de descrição do circuito, quando necessário, como `diagram.json`

Após a compilação, a simulação poderá ser iniciada diretamente pelo VS Code através dos comandos disponibilizados pela extensão Wokwi.

---

## 26.3. Fluxo recomendado

O fluxo completo de desenvolvimento fica:

```text
Editar código
      ↓
Salvar
      ↓
Terminal do Dev Container
      ↓
source /opt/esp/idf/export.sh
      ↓
idf.py build
      ↓
Arquivos gerados em build/
      ↓
Wokwi
      ↓
Simulação do ESP32
```

### Comandos principais

Dentro do terminal do **Dev Container**, execute:

```bash
source /opt/esp/idf/export.sh
```

Depois:

```bash
idf.py build
```

Após uma compilação bem-sucedida, os arquivos necessários para a simulação estarão disponíveis na pasta:

`build/`

---

## 26.4. Estrutura final do projeto

Com **Docker + Dev Containers + ESP-IDF + Wokwi**, a estrutura do projeto poderá ficar:

```text
Project_aula_2_IOT/
│
├── .devcontainer/
│   └── devcontainer.json
│
├── main/
│   ├── main.c
│   └── CMakeLists.txt
│
├── build/
│   ├── Project_aula_2_IOT.elf
│   ├── flasher_args.json
│   └── ...
│
├── diagram.json
├── wokwi.toml
├── CMakeLists.txt
└── README.md
```

### Arquivos principais

| Arquivo/Pasta | Função |
|---|---|
| `.devcontainer/` | Configuração do ambiente de desenvolvimento |
| `main/` | Código-fonte principal do ESP32 |
| `build/` | Arquivos gerados pelo `idf.py build` |
| `wokwi.toml` | Configuração da integração com o Wokwi |
| `diagram.json` | Descrição dos componentes e conexões do circuito |
| `CMakeLists.txt` | Configuração de compilação do projeto |
| `README.md` | Documentação do projeto |

> O arquivo `diagram.json` é utilizado quando a simulação precisa representar componentes e conexões do circuito.

---

## 26.5. Resultado esperado

Ao final desta etapa, o ambiente deverá funcionar da seguinte forma:

```text
VS Code
   │
   ├── Dev Container
   │      │
   │      └── ESP-IDF
   │             │
   │             └── idf.py build
   │                    │
   │                    └── build/
   │
   └── Wokwi
          │
          ├── wokwi.toml
          ├── diagram.json
          └── build/
                 │
                 └── Project_aula_2_IOT.elf
                            │
                            ↓
                     Simulação ESP32
```

Com isso, o projeto estará configurado para desenvolver, compilar e simular o ESP32 utilizando **VS Code, Docker, Dev Container, ESP-IDF e Wokwi**.
