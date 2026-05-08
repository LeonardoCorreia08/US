# Usar imagem base Python
FROM python:3.11-slim

# Definir diretório de trabalho
WORKDIR /app

# Instalar dependências do sistema
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copiar e instalar requisitos primeiro para cache
COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

# Copiar o restante do código
COPY . .

# Criar pastas que o script espera (figures, models, etc)
RUN mkdir -p reports/figures models data

# Expor a porta do Streamlit
EXPOSE 8501

# Comentário: Se você já tiver os modelos treinados localmente, 
# apenas o comando do Streamlit basta. 
# Caso precise treinar ao iniciar:
CMD ["sh", "-c", "python3 main.py && streamlit run app/app.py --server.port=8501 --server.address=0.0.0.0"]
