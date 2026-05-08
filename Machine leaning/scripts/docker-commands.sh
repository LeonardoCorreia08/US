#!/bin/bash
# Scripts de conveniência para Docker - Projeto Monitoramento Ambiental
# Autor: Adaptado para Leonardo Henrique

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🐳 Scripts Docker - Monitoramento de Qualidade Ambiental${NC}"

# Função para mostrar ajuda
show_help() {
    echo -e "${YELLOW}Modo de uso: ./docker-commands.sh [comando]${NC}"
    echo ""
    echo -e "${GREEN}🏗️  BUILD & RUN:${NC}"
    echo "  up             - Iniciar todos os serviços (Dashboard + MLflow)"
    echo "  build          - Reconstruir imagens Docker"
    echo "  down           - Parar e remover containers"
    echo ""
    echo -e "${GREEN}🎯 EXECUÇÃO DE PIPELINE:${NC}"
    echo "  train          - Executar treinamento (main.py) dentro do container"
    echo "  eda            - Executar apenas análise exploratória (src/eda.py)"
    echo ""
    echo -e "${GREEN}🔧 UTILITÁRIOS:${NC}"
    echo "  shell          - Abrir terminal bash no container do Dashboard"
    echo "  logs           - Ver logs em tempo real"
    echo "  clean          - Limpeza profunda (remove volumes e imagens)"
    echo ""
    echo -e "${GREEN}📊 MONITORAMENTO:${NC}"
    echo "  status         - Ver status dos serviços e portas"
    echo "  urls           - Mostrar links de acesso (Streamlit e MLflow)"
}

# Função para subir os serviços
docker_up() {
    echo -e "${BLUE}🚀 Iniciando serviços em background...${NC}"
    docker-compose up -d
    echo -e "${GREEN}✅ Serviços iniciados!${NC}"
    docker_urls
}

# Função para build
docker_build() {
    echo -e "${BLUE}🏗️  Construindo imagens do projeto...${NC}"
    docker-compose build
    echo -e "${GREEN}✅ Build concluído!${NC}"
}

# Função para treinamento
docker_train() {
    echo -e "${BLUE}🧠 Iniciando treinamento no container...${NC}"
    docker exec -it ambiental_streamlit python3 main.py
}

# Função para EDA
docker_eda() {
    echo -e "${BLUE}📊 Executando análise exploratória...${NC}"
    docker exec -it ambiental_streamlit python3 src/eda.py
}

# Mostrar URLs
docker_urls() {
    echo -e "${YELLOW}-------------------------------------------${NC}"
    echo -e "🖥️  Dashboard Streamlit: ${BLUE}http://localhost:8501${NC}"
    echo -e "📈 MLflow UI:          ${BLUE}http://localhost:5000${NC}"
    echo -e "${YELLOW}-------------------------------------------${NC}"
}

# Abrir shell
docker_shell() {
    echo -e "${BLUE}🐚 Abrindo shell no container ambiental_streamlit...${NC}"
    docker exec -it ambiental_streamlit bash
}

# Ver logs
docker_logs() {
    echo -e "${BLUE}📋 Visualizando logs (Ctrl+C para sair)...${NC}"
    docker-compose logs -f
}

# Limpeza profunda
docker_clean() {
    echo -e "${RED}⚠️  Limpando tudo (incluindo volumes de dados)...${NC}"
    docker-compose down --rmi all --volumes --remove-orphans
    echo -e "${GREEN}✅ Limpeza concluída!${NC}"
}

# Status
docker_status() {
    echo -e "${BLUE}📊 Status dos containers:${NC}"
    docker-compose ps
}

# Switch principal
case "$1" in
    "up")
        docker_up
        ;;
    "build")
        docker_build
        ;;
    "train")
        docker_train
        ;;
    "eda")
        docker_eda
        ;;
    "shell")
        docker_shell
        ;;
    "logs")
        docker_logs
        ;;
    "clean")
        docker_clean
        ;;
    "down")
        echo -e "${YELLOW}⏹️  Parando containers...${NC}"
        docker-compose down
        ;;
    "status")
        docker_status
        ;;
    "urls")
        docker_urls
        ;;
    *)
        show_help
        ;;
esac