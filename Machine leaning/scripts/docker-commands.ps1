# Scripts de conveniência para Docker no Windows PowerShell - Projeto Monitoramento Ambiental
# Autor: Adaptado para Leonardo Henrique

param(
    [Parameter(Position=0)]
    [string]$Command
)

function Show-Help {
    Write-Host " Scripts Docker - Monitoramento de Qualidade Ambiental (PowerShell)" -ForegroundColor Blue
    Write-Host ""
    Write-Host "Comandos disponíveis:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  BUILD & RUN:" -ForegroundColor Green
    Write-Host "  up             - Iniciar todos os serviços (Dashboard + MLflow)"
    Write-Host "  build          - Reconstruir imagens Docker"
    Write-Host "  down           - Parar e remover containers"
    Write-Host ""
    Write-Host " EXECUÇÃO DE PIPELINE:" -ForegroundColor Green
    Write-Host "  train          - Executar treinamento (main.py) dentro do container"
    Write-Host "  eda            - Executar apenas análise exploratória (src/eda.py)"
    Write-Host ""
    Write-Host " UTILITÁRIOS:" -ForegroundColor Green
    Write-Host "  shell          - Abrir shell bash no container do Dashboard"
    Write-Host "  logs           - Ver logs de todos os serviços"
    Write-Host "  clean          - Limpeza profunda (remove volumes e imagens)"
    Write-Host ""
    Write-Host " MONITORAMENTO:" -ForegroundColor Green
    Write-Host "  status         - Status dos containers e portas"
    Write-Host "  urls           - Mostrar links de acesso local"
    Write-Host ""
    Write-Host "Exemplos:" -ForegroundColor Cyan
    Write-Host "  .\docker-commands.ps1 up"
    Write-Host "  .\docker-commands.ps1 train"
}

function Docker-Up {
    Write-Host " Iniciando serviços em background..." -ForegroundColor Blue
    docker-compose up -d
    if ($LASTEXITCODE -eq 0) {
        Write-Host " Serviços iniciados!" -ForegroundColor Green
        Docker-Urls
    }
}

function Docker-Build {
    Write-Host "  Construindo imagens do projeto..." -ForegroundColor Blue
    docker-compose build
}

function Docker-Train {
    Write-Host " Iniciando treinamento no container ambiental_streamlit..." -ForegroundColor Blue
    docker exec -it ambiental_streamlit python3 main.py
}

function Docker-Eda {
    Write-Host " Executando análise exploratória..." -ForegroundColor Blue
    docker exec -it ambiental_streamlit python3 src/eda.py
}

function Docker-Urls {
    Write-Host "`n-------------------------------------------" -ForegroundColor Yellow
    Write-Host "  Dashboard Streamlit: http://localhost:8501" -ForegroundColor Blue
    Write-Host " MLflow UI:          http://localhost:5000" -ForegroundColor Blue
    Write-Host "-------------------------------------------`n" -ForegroundColor Yellow
}

function Docker-Shell {
    Write-Host " Abrindo shell no container ambiental_streamlit..." -ForegroundColor Blue
    docker exec -it ambiental_streamlit bash
}

function Docker-Logs {
    Write-Host " Visualizando logs (Pressione Ctrl+C para sair)..." -ForegroundColor Blue
    docker-compose logs -f
}

function Docker-Clean {
    Write-Host " Limpando containers, volumes e imagens..." -ForegroundColor Yellow
    docker-compose down --rmi all --volumes --remove-orphans
    Write-Host " Limpeza concluída!" -ForegroundColor Green
}

function Docker-Status {
    Write-Host " Status dos containers:" -ForegroundColor Blue
    docker-compose ps
}

# Switch principal
if ([string]::IsNullOrEmpty($Command)) {
    Show-Help
} else {
    switch ($Command.ToLower()) {
        "up"     { Docker-Up }
        "build"  { Docker-Build }
        "train"  { Docker-Train }
        "eda"    { Docker-Eda }
        "shell"  { Docker-Shell }
        "logs"   { Docker-Logs }
        "clean"  { Docker-Clean }
        "down"   { docker-compose down }
        "status" { Docker-Status }
        "urls"   { Docker-Urls }
        default  { Show-Help }
    }
}