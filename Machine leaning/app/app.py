import streamlit as st
import pandas as pd
import numpy as np
import joblib
import os
import yaml
import plotly.express as px
from sklearn.metrics import classification_report, confusion_matrix
import seaborn as sns
import matplotlib.pyplot as plt

# Configuração da página
st.set_page_config(page_title="Monitoramento de Qualidade Ambiental", layout="wide")

# Carregar configurações
def load_config():
    with open("config.yaml", 'r') as f:
        return yaml.safe_load(f)

config = load_config()

# Carregar modelos e transformadores
@st.cache_resource
def load_models():
    models_path = config['paths']['models']
    pipeline = joblib.load(os.path.join(models_path, 'preprocessing_pipeline.joblib'))
    label_encoder = joblib.load(os.path.join(models_path, 'label_encoder.joblib'))
    model = joblib.load(os.path.join(models_path, 'best_model.joblib'))
    feature_names = joblib.load(os.path.join(models_path, 'feature_names.joblib'))
    return pipeline, label_encoder, model, feature_names

try:
    pipeline, label_encoder, model, feature_names = load_models()
except Exception as e:
    st.error(f"Erro ao carregar modelos: {e}. Certifique-se de que o pipeline de treinamento foi executado.")
    st.stop()

# Sidebar
st.sidebar.title("Configurações")
app_mode = st.sidebar.selectbox("Escolha o modo", ["Dashboard", "Previsão Individual", "Previsão em Lote"])

# Aviso obrigatório
st.warning("Este conteúdo é destinado apenas para fins educacionais. Os dados exibidos são ilustrativos e podem não corresponder a situações reais.")

if app_mode == "Dashboard":
    st.title(" Dashboard de Qualidade Ambiental")
    
    # 1. Carregar dados
    df = pd.read_csv(config["data"]["raw_path"])
    
    # 2. LIMPEZA CRUCIAL: Converte textos como 'erro_sensor' em NaN (vazio)
    # Isso impede o erro na hora de calcular a correlação (corr)
    for col in config["features"]["numerical"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    # 3. Remover colunas de identificação
    columns_to_drop_for_viz = ["Sensor_Status", "Sensor_ID", "Heat_Index"]
    df_viz = df.drop(columns=[col for col in columns_to_drop_for_viz if col in df.columns], errors='ignore')

    col1, col2 = st.columns(2)
    
    with col1:
        st.subheader("Distribuição da Qualidade Ambiental")
        fig = px.pie(df_viz, names=config["data"]["target"], title="Classes de Qualidade")
        st.plotly_chart(fig)
        
    with col2:
        st.subheader("Correlação entre Variáveis")
        # Adicionamos numeric_only=True por segurança extra
        corr = df_viz.drop(columns=[config["data"]["target"]]).corr(numeric_only=True)
        fig_corr = px.imshow(corr, text_auto=True, aspect="auto", title="Matriz de Correlação")
        st.plotly_chart(fig_corr)
        
    st.subheader("Distribuição de Variáveis por Qualidade")
    feature = st.selectbox("Selecione uma variável para visualizar", [col for col in config["features"]["numerical"] if col in df_viz.columns])
    fig_box = px.box(df_viz, x=config["data"]["target"], y=feature, color=config["data"]["target"], title=f"Distribuição de {feature} por Classe")
    st.plotly_chart(fig_box)

elif app_mode == "Previsão Individual":
    st.title(" Previsão Individual")
    st.write("Insira os valores das variáveis ambientais para prever a qualidade.")
    
    input_data = {}
    cols = st.columns(4)
    
    for i, feature in enumerate(feature_names):
        with cols[i % 4]:
            input_data[feature] = st.number_input(f"{feature}", value=0.0)
            
    if st.button("Prever Qualidade"):
        input_df = pd.DataFrame([input_data])
        # Garantir que as colunas de entrada estejam na ordem correta
        input_df = input_df[feature_names]
        X_scaled = pipeline.transform(input_df)
        prediction_encoded = model.predict(X_scaled)
        prediction = label_encoder.inverse_transform(prediction_encoded)[0]
        
        st.success(f"A Qualidade Ambiental prevista é: **{prediction}**")
        
        # Mostrar probabilidades
        probs = model.predict_proba(X_scaled)[0]
        prob_df = pd.DataFrame({
            'Classe': label_encoder.classes_,
            'Probabilidade': probs
        })
        fig_prob = px.bar(prob_df, x='Classe', y='Probabilidade', title="Probabilidades por Classe")
        st.plotly_chart(fig_prob)

elif app_mode == "Previsão em Lote":
    st.title(" Previsão em Lote")
    uploaded_file = st.file_uploader("Faça upload de um arquivo CSV", type="csv")
    
    if uploaded_file is not None:
        batch_df = pd.read_csv(uploaded_file)
        
        if st.button("Processar Lote"):
            try:
                # 1. Seleciona as colunas numéricas configuradas
                expected_features = config["features"]["numerical"]
                batch_df_filtered = batch_df[expected_features].copy()
                
                # 2. A CORREÇÃO: Converte tudo para número. 
                # O que for 'erro_sensor' vira NaN automaticamente.
                for col in expected_features:
                    batch_df_filtered[col] = pd.to_numeric(batch_df_filtered[col], errors='coerce')
                
                # 3. Agora o pipeline funciona! 
                # O SimpleImputer verá o NaN e colocará a mediana no lugar.
                X_batch = pipeline.transform(batch_df_filtered)
                preds_encoded = model.predict(X_batch)
                preds = label_encoder.inverse_transform(preds_encoded)
                
                # 4. Exibe e permite baixar os resultados
                batch_df['Previsão_Qualidade'] = preds
                st.write("Resultados:")
                st.dataframe(batch_df)
                
                csv = batch_df.to_csv(index=False).encode('utf-8')
                st.download_button("Baixar Resultados", csv, "previsoes.csv", "text/csv")
                
            except Exception as e:
                st.error(f"Erro ao processar lote: {e}")
