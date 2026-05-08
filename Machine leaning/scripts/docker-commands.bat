@echo off
REM Scripts de conveniência para Docker - Projeto Monitoramento Ambiental (Leonardo Henrique)
cd /d "%~dp0"
chcp 65001 > nul

echo  Scripts Docker - Monitoramento Ambiental (Windows)

if "%1"=="" goto show_help
if "%1"=="up" goto docker_up
if "%1"=="build" goto docker_build
if "%1"=="train" goto docker_train
if "%1"=="eda" goto docker_eda
if "%1"=="shell" goto docker_shell
if "%1"=="logs" goto docker_logs
if "%1"=="clean" goto docker_clean
if "%1"=="stop" goto docker_stop
if "%1"=="status" goto docker_status
if "%1"=="urls" goto docker_urls
goto show_help

:show_help
echo.
echo Comandos disponíveis:
echo.
echo   ESTRUTURA:
echo   up             - Iniciar Dashboard e MLflow (Background)
echo   build          - Reconstruir imagens Docker
echo   stop           - Parar containers
echo   clean          - Limpeza profunda (remove volumes e imagens)
echo.
echo  EXECUÇÃO:
echo   train          - Rodar treinamento (main.py) no container
echo   eda            - Rodar análise exploratória (eda.py) no container
echo.
echo  UTILITÁRIOS:
echo   shell          - Abrir terminal no container do Dashboard
echo   logs           - Ver logs em tempo real
echo   status         - Ver status dos serviços
echo   urls           - Mostrar links de acesso
echo.
echo Exemplos:
echo   docker-commands.bat up
echo   docker-commands.bat train
goto end

:docker_up
echo  Iniciando serviços (Streamlit + MLflow)...
docker-compose up -d
echo  Serviços iniciados!
goto docker_urls

:docker_build
echo   Construindo imagens do projeto...
docker-compose build
goto end

:docker_train
echo  Iniciando treinamento dentro do container...
docker exec -it ambiental_streamlit python3 main.py
goto end

:docker_eda
echo  Executando análise exploratória...
docker exec -it ambiental_streamlit python3 src/eda.py
goto end

:docker_urls
echo.
echo -------------------------------------------
echo   Dashboard Streamlit: http://localhost:8501
echo  MLflow UI:          http://localhost:5000
echo -------------------------------------------
echo.
goto end

:docker_shell
echo  Abrindo shell no container ambiental_streamlit...
docker exec -it ambiental_streamlit bash
goto end

:docker_logs
echo  Visualizando logs (Ctrl+C para sair)...
docker-compose logs -f
goto end

:docker_clean
echo  Limpando containers e imagens...
docker-compose down --rmi all --volumes --remove-orphans
echo  Limpeza concluída!
goto end

:docker_stop
echo ⏹  Parando containers...
docker-compose down
goto end

:docker_status
echo  Status dos containers:
docker-compose ps
goto end

:end