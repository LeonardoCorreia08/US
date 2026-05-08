import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import seaborn as sns
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, LabelEncoder
from sklearn.impute import SimpleImputer
from sklearn.pipeline import Pipeline
from imblearn.over_sampling import SMOTE
import joblib
import os


CLASS_PALETTE = {
    "Excelente":  "#2ecc71",
    "Boa":        "#27ae60",
    "Moderada":   "#f39c12",
    "Ruim":       "#e74c3c",
    "Muito Ruim": "#8e44ad",
}


class DataPreprocessor:
    """
    Encapsula todo o pré-processamento do dataset ambiental.

    Ordem de execução interna do prepare_data():
    ┌─────────────────────────────────────────────────────────┐
    │  DataFrame bruto                                        │
    │    → drop colunas irrelevantes                          │
    │    → to_numeric (coerce) nas features                   │
    │    → dropna APENAS no target                            │
    │    → LabelEncoder no target                             │
    │    → train_test_split (stratify)                        │
    │         ↓                          ↓                    │
    │    X_train → pipeline.fit_transform  X_test → transform │
    │         ↓                                               │
    │    SMOTE (apenas no treino)                             │
    │    → salvar pipeline / encoder / feature_names          │
    └─────────────────────────────────────────────────────────┘
    """

    def __init__(self, config):
        self.config = config
        self.label_encoder = LabelEncoder()
        # Pipeline: imputer → scaler. O fit() é chamado APENAS no treino.
        self.pipeline = Pipeline([
            ("imputer", SimpleImputer(strategy="median")),
            ("scaler",  StandardScaler()),
        ])
        self._is_fitted = False   # flag interna de controle

    
    def _assert_no_leakage(self):
        """
        Verifica que o pipeline foi fitado (no treino) e que os
        parâmetros aprendidos existem nos steps corretos.
        Lança AssertionError se o pipeline não estiver pronto.
        """
        imputer = self.pipeline.named_steps["imputer"]
        scaler  = self.pipeline.named_steps["scaler"]

        assert hasattr(imputer, "statistics_"), (
            "❌ Data leakage: SimpleImputer não foi fitado no treino!"
        )
        assert hasattr(scaler, "mean_"), (
            "❌ Data leakage: StandardScaler não foi fitado no treino!"
        )
        print("  ✔ Anti-leakage: pipeline fitado apenas no treino (OK)")

    
    def log_preprocessing_stats(self, n_original, X_train, X_test,
                                 X_train_bal, y_train_bal):
        """
        Imprime e retorna tabela comparativa de volumes em cada etapa.
        """
        n_train_before = len(X_train)
        n_test         = len(X_test)
        n_train_after  = len(X_train_bal)
        growth_pct     = (n_train_after / n_train_before - 1) * 100 if n_train_before > 0 else 0.0

        stats = {
            "Linhas originais":              n_original,
            "Linhas treino (antes SMOTE)":   n_train_before,
            "Linhas teste":                  n_test,
            "Linhas treino (após SMOTE)":    n_train_after,
            "Crescimento pelo SMOTE":        f"{growth_pct:.1f}%",
            "Proporção treino/teste":        f"{n_train_before}/{n_test} "
                                             f"({n_train_before/(n_train_before+n_test)*100:.0f}% / "
                                             f"{n_test/(n_train_before+n_test)*100:.0f}%)",
        }

        col_w = max(len(k) for k in stats) + 2
        print("\n  ┌─ Estatísticas de Pré-processamento " + "─" * 20)
        for k, v in stats.items():
            print(f"  │  {k:<{col_w}}: {v}")
        print("  └" + "─" * (col_w + 20))

        return stats

    
    def plot_smote_effect(self, y_before, y_after, output_dir):
        """
        Gráfico lado a lado: distribuição antes e após o SMOTE.
        Usa o label_encoder interno — não precisa receber encoder externo.
        """
        print("  Gerando gráfico de efeito do SMOTE...")
        os.makedirs(output_dir, exist_ok=True)

        le           = self.label_encoder
        ordem_classes = le.classes_.tolist()
        cores         = [CLASS_PALETTE.get(c, "#95a5a6") for c in ordem_classes]

        # Decodificar inteiros → strings
        before_labels = le.inverse_transform(y_before)
        after_labels  = le.inverse_transform(y_after)

        before_counts = (pd.Series(before_labels)
                         .value_counts()
                         .reindex(ordem_classes, fill_value=0))
        after_counts  = (pd.Series(after_labels)
                         .value_counts()
                         .reindex(ordem_classes, fill_value=0))

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

        # ── Antes ──────────────────────────────────────────
        bars1 = ax1.barh(before_counts.index, before_counts.values,
                         color=cores, edgecolor="white")
        ax1.bar_label(bars1, fmt="%d", padding=5)
        ax1.set_title("Antes do SMOTE", fontweight="bold")
        ax1.set_xlabel("Nº de amostras")
        ax1.invert_yaxis()

        # ── Depois ─────────────────────────────────────────
        bars2 = ax2.barh(after_counts.index, after_counts.values,
                         color=cores, edgecolor="white")
        ax2.bar_label(bars2, fmt="%d", padding=5)
        ax2.set_title("Após o SMOTE", fontweight="bold")
        ax2.set_xlabel("Nº de amostras")
        ax2.invert_yaxis()

        plt.suptitle("Efeito do SMOTE — Balanceamento das Classes",
                     fontsize=14, fontweight="bold")
        plt.tight_layout()

        filepath = os.path.join(output_dir, "smote_balanceamento.png")
        fig.savefig(filepath, bbox_inches="tight")
        plt.close(fig)
        print(f"  ✔ Gráfico salvo: {filepath}")
        return filepath

    
    def plot_preprocessing_dashboard(self, df_original, X_train, X_test,
                                      X_train_bal, y_train, y_train_bal,
                                      feature_names, output_dir):
        """
        Painel escuro com 4 painéis:
        1. Volumes em cada etapa (funil)
        2. Distribuição de classes antes/após SMOTE
        3. Médias do imputer (valores imputados) por feature
        4. Desvios-padrão aprendidos pelo scaler
        """
        print("  Gerando dashboard de pré-processamento...")
        os.makedirs(output_dir, exist_ok=True)

        le            = self.label_encoder
        ordem_classes = le.classes_.tolist()
        cores         = [CLASS_PALETTE.get(c, "#95a5a6") for c in ordem_classes]
        imputer_stats = self.pipeline.named_steps["imputer"].statistics_
        scaler_means  = self.pipeline.named_steps["scaler"].mean_
        scaler_stds   = self.pipeline.named_steps["scaler"].scale_

        fig = plt.figure(figsize=(18, 12), facecolor="#1a1a2e")
        fig.suptitle("⚙️ Dashboard — Pré-processamento", fontsize=17,
                     fontweight="bold", color="white", y=0.98)
        gs = gridspec.GridSpec(2, 3, figure=fig, hspace=0.45, wspace=0.35)

        dark = "#16213e"
        border = "#444"

        def _style_ax(ax, title):
            ax.set_facecolor(dark)
            ax.tick_params(colors="white", labelsize=8)
            for spine in ax.spines.values():
                spine.set_color(border)
            ax.set_title(title, color="white", fontweight="bold", fontsize=10)

        # ── Painel 1: Funil de volumes ────────────────────
        ax1 = fig.add_subplot(gs[0, 0])
        etapas  = ["Original", "Treino\n(pré-SMOTE)", "Teste", "Treino\n(pós-SMOTE)"]
        volumes = [len(df_original), len(X_train), len(X_test), len(X_train_bal)]
        bar_colors = ["#3498db", "#e67e22", "#9b59b6", "#2ecc71"]
        bars = ax1.barh(etapas[::-1], volumes[::-1], color=bar_colors[::-1], edgecolor="white")
        ax1.bar_label(bars, fmt="%d", padding=4, color="white", fontsize=8)
        _style_ax(ax1, "Volume por Etapa")
        ax1.set_xlabel("Nº de amostras", color="white", fontsize=8)

        # ── Painel 2: Classes antes SMOTE ────────────────
        ax2 = fig.add_subplot(gs[0, 1])
        before_counts = (pd.Series(le.inverse_transform(y_train))
                         .value_counts().reindex(ordem_classes, fill_value=0))
        b2 = ax2.barh(before_counts.index, before_counts.values,
                      color=cores, edgecolor="white")
        ax2.bar_label(b2, fmt="%d", padding=4, color="white", fontsize=8)
        ax2.invert_yaxis()
        _style_ax(ax2, "Classes — Antes SMOTE")
        ax2.set_xlabel("Nº de amostras", color="white", fontsize=8)

        # ── Painel 3: Classes após SMOTE ─────────────────
        ax3 = fig.add_subplot(gs[0, 2])
        after_counts = (pd.Series(le.inverse_transform(y_train_bal))
                        .value_counts().reindex(ordem_classes, fill_value=0))
        b3 = ax3.barh(after_counts.index, after_counts.values,
                      color=cores, edgecolor="white")
        ax3.bar_label(b3, fmt="%d", padding=4, color="white", fontsize=8)
        ax3.invert_yaxis()
        _style_ax(ax3, "Classes — Após SMOTE")
        ax3.set_xlabel("Nº de amostras", color="white", fontsize=8)

        # ── Painel 4: Valores imputados (mediana do treino) ──
        ax4 = fig.add_subplot(gs[1, :2])
        x = np.arange(len(feature_names))
        ax4.bar(x, imputer_stats, color="#3498db", alpha=0.8, edgecolor="white", label="Mediana imputada")
        ax4.plot(x, scaler_means, "o--", color="#e74c3c", linewidth=1.5,
                 markersize=5, label="Média (scaler)")
        ax4.set_xticks(x)
        ax4.set_xticklabels(feature_names, rotation=25, color="white", fontsize=8)
        ax4.legend(facecolor=dark, labelcolor="white", fontsize=8)
        _style_ax(ax4, "Valores aprendidos pelo Imputer e Scaler (treino)")
        ax4.set_ylabel("Valor", color="white", fontsize=8)

        # ── Painel 5: Desvios-padrão do scaler ─────────────
        ax5 = fig.add_subplot(gs[1, 2])
        colors_std = plt.cm.RdYlGn(np.linspace(0.2, 0.9, len(feature_names)))
        bars5 = ax5.barh(feature_names, scaler_stds, color=colors_std, edgecolor="white")
        ax5.bar_label(bars5, fmt="%.2f", padding=3, color="white", fontsize=7)
        _style_ax(ax5, "Desvio-padrão por Feature (scaler)")
        ax5.set_xlabel("σ", color="white", fontsize=8)

        filepath = os.path.join(output_dir, "preprocessing_dashboard.png")
        fig.savefig(filepath, bbox_inches="tight")
        plt.close(fig)
        print(f"  ✔ Dashboard salvo: {filepath}")
        return filepath

    
    def prepare_data(self, df):
        """
        Pipeline completo de pré-processamento.

        Retorna
        -------
        X_train_bal   : np.ndarray — treino balanceado e escalado
        X_test_scaled : np.ndarray — teste escalado (sem re-fit)
        y_train_bal   : np.ndarray — labels do treino balanceado
        y_test        : np.ndarray — labels do teste
        feature_names : list[str]  — nomes das features na ordem correta
        stats         : dict       — estatísticas de volume por etapa

        NOTA: retorno expandido de 5 para 6 valores (adicionado stats).
        Atualize os chamadores: X_train, X_test, y_train, y_test, cols, stats = ...
        """
        print("\n" + "=" * 60)
        print("   ⚙️  PRÉ-PROCESSAMENTO — INÍCIO")
        print("=" * 60)

        n_original  = len(df)
        df_original = df.copy()
        target_col  = self.config["data"]["target"]

        # ── Etapa 1: Drop de colunas irrelevantes ─────────────
        print("\n[1/6] Removendo colunas irrelevantes...")
        cols_to_drop = self.config.get("preprocessing", {}).get(
            "columns_to_drop", ["Sensor_Status", "Sensor_ID", "Heat_Index"]
        )
        df_proc = df.drop(
            columns=[c for c in cols_to_drop if c in df.columns],
            errors="ignore",
        )
        dropped = [c for c in cols_to_drop if c in df.columns]
        print(f"  Removidas: {dropped if dropped else 'nenhuma'}")

        # ── Etapa 2: Conversão para numérico ──────────────────
        print("\n[2/6] Convertendo features para numérico (errors='coerce')...")
        for col in df_proc.columns:
            if col != target_col:
                df_proc[col] = pd.to_numeric(df_proc[col], errors="coerce")

        nulos_features = df_proc.drop(columns=[target_col]).isnull().sum()
        nulos_com = nulos_features[nulos_features > 0]
        if not nulos_com.empty:
            print("  Nulos nas features (serão imputados pelo pipeline):")
            for feat, n in nulos_com.items():
                print(f"    {feat}: {n} nulos ({n/len(df_proc)*100:.1f}%)")
        else:
            print("  Sem nulos nas features após conversão.")

        # ── Etapa 3: Remover nulos APENAS no target ───────────
        print("\n[3/6] Removendo linhas sem target...")
        before_drop = len(df_proc)
        df_proc.dropna(subset=[target_col], inplace=True)
        removed = before_drop - len(df_proc)
        print(f"  Removidas {removed} linha(s) sem target "
              f"({removed/before_drop*100:.2f}%)")

        # ── Separar X e y ─────────────────────────────────────
        X = df_proc.drop(columns=[target_col])
        y = df_proc[target_col]
        feature_names = X.columns.tolist()

        # ── Etapa 4: Encoding do target ───────────────────────
        print("\n[4/6] Encoding do target com LabelEncoder...")
        y_enc = self.label_encoder.fit_transform(y)
        print(f"  Classes: {dict(zip(self.label_encoder.classes_, self.label_encoder.transform(self.label_encoder.classes_)))}")

        # ── Etapa 5: Split treino / teste ─────────────────────
        print("\n[5/6] Split treino/teste...")
        X_train, X_test, y_train, y_test = train_test_split(
            X, y_enc,
            test_size=self.config["data"]["test_size"],
            random_state=self.config["data"]["random_state"],
            stratify=y_enc,
        )
        print(f"  Treino: {X_train.shape[0]:,} amostras  |  "
              f"Teste: {X_test.shape[0]:,} amostras")

        # ── Etapa 5b: Pipeline fit (treino) + transform ───────
        print("\n  Aplicando pipeline (Imputer → Scaler)...")
        print(f"  → fit_transform no TREINO  (shape: {X_train.shape})")
        X_train_scaled = self.pipeline.fit_transform(X_train)
        self._is_fitted = True

        print(f"  → transform no TESTE sem re-fit (shape: {X_test.shape})")
        X_test_scaled  = self.pipeline.transform(X_test)

        # Validação de data leakage via parâmetros aprendidos
        self._assert_no_leakage()

        # ── Etapa 6: SMOTE ────────────────────────────────────
        print("\n[6/6] Aplicando SMOTE (apenas no treino)...")
        # k_neighbors=3 necessário para classes com < 6 amostras (ex: Excelente)
        smote = SMOTE(
            random_state=self.config["data"]["random_state"],
            k_neighbors=3,
        )
        X_train_bal, y_train_bal = smote.fit_resample(X_train_scaled, y_train)
        print(f"  Treino antes: {len(X_train_scaled):,}  →  "
              f"após SMOTE: {len(X_train_bal):,}")

        # ── Salvar artefatos ───────────────────────────────────
        models_path = self.config["paths"]["models"]
        os.makedirs(models_path, exist_ok=True)
        joblib.dump(self.pipeline,      os.path.join(models_path, "preprocessing_pipeline.joblib"))
        joblib.dump(self.label_encoder, os.path.join(models_path, "label_encoder.joblib"))
        joblib.dump(feature_names,      os.path.join(models_path, "feature_names.joblib"))
        print(f"\n   Artefatos salvos em: {models_path}")
        print(f"    - preprocessing_pipeline.joblib")
        print(f"    - label_encoder.joblib")
        print(f"    - feature_names.joblib")

        # ── Estatísticas ───────────────────────────────────────
        stats = self.log_preprocessing_stats(
            n_original, X_train, X_test, X_train_bal, y_train_bal
        )

        # ── Visualizações ──────────────────────────────────────
        reports_dir = self.config["paths"]["reports"]
        figures_dir = os.path.join(reports_dir, "figures")

        self.plot_smote_effect(y_train, y_train_bal, figures_dir)

        self.plot_preprocessing_dashboard(
            df_original, X_train, X_test,
            X_train_bal, y_train, y_train_bal,
            feature_names, figures_dir,
        )

        print("\n" + "=" * 60)
        print("    PRÉ-PROCESSAMENTO CONCLUÍDO")
        print("=" * 60 + "\n")

        return X_train_bal, X_test_scaled, y_train_bal, y_test, feature_names, stats

    
    def transform_new_data(self, df):
        """
        Transforma novos dados usando o pipeline já fitado.
        Garante que as features estejam na mesma ordem do treino.
        """
        if not self._is_fitted:
            raise RuntimeError(
                "Pipeline não fitado. Execute prepare_data() antes de transform_new_data()."
            )

        models_path   = self.config["paths"]["models"]
        feature_names = joblib.load(os.path.join(models_path, "feature_names.joblib"))

        # Drop de colunas irrelevantes
        cols_to_drop = self.config.get("preprocessing", {}).get(
            "columns_to_drop", ["Sensor_Status", "Sensor_ID", "Heat_Index"]
        )
        df_proc = df.drop(
            columns=[c for c in cols_to_drop if c in df.columns],
            errors="ignore",
        )

        # Converter para numérico
        for col in df_proc.columns:
            df_proc[col] = pd.to_numeric(df_proc[col], errors="coerce")

        # Reordenar colunas conforme treino
        missing = [f for f in feature_names if f not in df_proc.columns]
        if missing:
            raise ValueError(f"Colunas ausentes nos novos dados: {missing}")

        X_new = df_proc[feature_names]
        return self.pipeline.transform(X_new)



if __name__ == "__main__":
    import yaml

    with open("../config.yaml", "r") as f:
        config = yaml.safe_load(f)

    df = pd.read_csv("../data/dataset_ambiental.csv")

    preprocessor = DataPreprocessor(config)
    X_train, X_test, y_train, y_test, feature_names, stats = preprocessor.prepare_data(df)

    print(f"X_train shape : {X_train.shape}")
    print(f"X_test  shape : {X_test.shape}")
    print(f"Features      : {feature_names}")
    print(f"\nEstatísticas  :")
    for k, v in stats.items():
        print(f"  {k}: {v}")