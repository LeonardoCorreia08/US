@echo off
cd /d "%~dp0"
chcp 65001 > nul
REM Quick Start Automático - Projeto Monitoramento Ambiental (Leonardo Henrique)

echo.
echo  ================================================
echo   QUICK START - Setup Ambiental Docker + MLflow
echo  ================================================

REM Verificar se está executando como Admin
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERRO] Execute este script como Administrador!
    pause
    exit /b 1
)

echo.
echo 📁 Criando e verificando estrutura de diretórios...
if not exist "data" mkdir data
if not exist "models" mkdir models
if not exist "reports\figures" mkdir reports\figures
if not exist "mlruns" mkdir mlruns

echo.
echo 🔍 Verificando Docker e Compose...
docker --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERRO] Docker não encontrado! Instale o Docker Desktop.
    pause
    exit /b 1
)

echo.
echo 📄 Verificando arquivos do projeto...
set missing_files=0

if not exist "Dockerfile" (echo  [!] Dockerfile faltando & set missing_files=1)
if not exist "docker-compose.yml" (echo  [!] docker-compose.yml faltando & set missing_files=1)
if not exist "requirements.txt" (echo  [!] requirements.txt faltando & set missing_files=1)
if not exist "config.yaml" (echo  [!] config.yaml faltando & set missing_files=1)
if not exist "app\app.py" (echo  [!] app/app.py faltando & set missing_files=1)

if %missing_files% == 1 (
    echo.
    echo [ERRO] Arquivos críticos faltando! Verifique a pasta do projeto.
    pause
    exit /b 1
)

echo.
echo 🚀 Iniciando Docker Desktop (se necessário)...
start "" "C:\Program Files\Docker\Docker\Docker Desktop.exe"
timeout /t 10 /nobreak >nul

:wait_docker
docker info >nul 2>&1
if %errorLevel% neq 0 (
    echo   Aguardando inicialização do motor do Docker...
    timeout /t 5 /nobreak >nul
    goto wait_docker
)

echo.
echo 🛠️  Construindo e subindo os containers...
echo    (Isso pode demorar na primeira vez)
docker-compose up -d --build

if %errorLevel% neq 0 (
    echo [ERRO] Falha ao iniciar o Docker Compose.
    pause
    exit /b 1
)

echo.
echo ================================================
echo   SISTEMA INICIALIZADO COM SUCESSO!
echo ================================================
echo.
echo 1 - Abrir Dashboard (Streamlit)
echo 2 - Abrir MLflow (Experimentos)
echo 3 - Rodar Novo Treinamento (main.py)
echo 4 - Ver Logs dos Containers
echo 5 - Parar Tudo e Sair
echo.

set /p choice="Escolha uma opção (1-5): "

if "%choice%"=="1" (
    echo Abrindo Dashboard...
    start http://localhost:8501
    goto end_script
) else if "%choice%"=="2" (
    echo Abrindo MLflow...
    start http://localhost:5000
    goto end_script
) else if "%choice%"=="3" (
    echo Executando pipeline de treinamento dentro do container...
    docker exec -it ambiental_streamlit python3 main.py
    pause
) else if "%choice%"=="4" (
    docker-compose logs -f
) else if "%choice%"=="5" (
    echo Parando serviços...
    docker-compose down
    exit /b 0
)

:end_script
echo.
echo O sistema continua rodando em background.
echo Para encerrar depois, use: docker-compose down
echo.
pause