import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import seaborn as sns
import mlflow
import mlflow.sklearn
import optuna
from datetime import datetime
from lazypredict.Supervised import LazyClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import (
    accuracy_score, precision_score, recall_score, f1_score,
    confusion_matrix, classification_report, ConfusionMatrixDisplay,
)
from sklearn.model_selection import StratifiedKFold, cross_val_score
import joblib
import os

# ─────────────────────────────────────────────────────────────────
# CONFIGURAÇÕES GLOBAIS
# ─────────────────────────────────────────────────────────────────
optuna.logging.set_verbosity(optuna.logging.WARNING)

# Paleta centralizada — mesma do eda.py e preprocessing.py
CLASS_PALETTE = {
    "Excelente":  "#2ecc71",
    "Boa":        "#27ae60",
    "Moderada":   "#f39c12",
    "Ruim":       "#e74c3c",
    "Muito Ruim": "#8e44ad",
}


class ModelTrainer:
    """
    Encapsula benchmark, otimização, treinamento final e rastreamento MLflow.

    Fluxo principal:
    ┌──────────────────────────────────────────────────────────────┐
    │  run_benchmark()          → LazyPredict (baseline rápido)   │
    │  optimize_hyperparameters() → Optuna + StratifiedKFold      │
    │  train_final_model()      → RF final + MLflow + artefatos   │
    │  verify_saved_artifacts() → smoke test de inferência        │
    └──────────────────────────────────────────────────────────────┘
    """

    def __init__(self, config):
        self.config      = config
        self.figures_dir = os.path.join(
            config["paths"]["reports"], "figures"
        )
        os.makedirs(self.figures_dir, exist_ok=True)

        # MLflow
        mlflow.set_tracking_uri(config["mlflow"]["tracking_uri"])
        exp_name = config["mlflow"]["experiment_name"]
        if mlflow.get_experiment_by_name(exp_name) is None:
            mlflow.create_experiment(exp_name)
        mlflow.set_experiment(exp_name)

    
    def _save_fig(self, fig, filename):
        """Salva figura matplotlib em self.figures_dir e fecha."""
        os.makedirs(self.figures_dir, exist_ok=True)
        filepath = os.path.join(self.figures_dir, filename)
        fig.savefig(filepath, bbox_inches="tight")
        plt.close(fig)
        print(f"  ✔ Gráfico salvo: {filepath}")
        return filepath

    
    def plot_benchmark_ranking(self, models_df):
        """
        Barplot horizontal com top 15 modelos por Accuracy.
        CORREÇÃO: LazyPredict retorna 'Model' como índice, não coluna.
        reset_index() é obrigatório antes do barplot.
        """
        print("  Gerando ranking de modelos do LazyPredict...")

        # ── reset_index traz 'Model' para coluna ──────────────
        top15 = (
            models_df
            .sort_values("Accuracy", ascending=False)
            .head(15)
            .reset_index()          # ← correção crítica
        )

        fig, ax = plt.subplots(figsize=(12, 7))
        colors  = plt.cm.RdYlGn(np.linspace(0.3, 0.9, len(top15)))
        bars    = ax.barh(top15["Model"][::-1], top15["Accuracy"][::-1],
                          color=colors[::-1], edgecolor="white")
        ax.bar_label(bars, fmt="%.3f", padding=5)
        ax.axvline(0.5, color="red", linestyle="--", alpha=0.6, label="Baseline 50%")
        ax.set_xlim(0, 1.12)
        ax.set_title(" Top 15 Modelos — LazyPredict", fontweight="bold", fontsize=14)
        ax.set_xlabel("Accuracy")
        ax.legend()
        plt.tight_layout()
        return self._save_fig(fig, "lazypredict_ranking.png")

    def run_benchmark(self, X_train, X_test, y_train, y_test):
        """
        Benchmark com LazyPredict usando amostra de até 3000 amostras.
        Salva CSV e gráfico de ranking.
        """
        print("\n" + "=" * 60)
        print("    BENCHMARK — LazyPredict")
        print("=" * 60)

        mlflow.sklearn.autolog(disable=True)

        # Amostra para não travar com dados balanceados pelo SMOTE
        n_lazy = min(3000, len(X_train))
        rng    = np.random.default_rng(self.config["data"]["random_state"])
        idx    = rng.choice(len(X_train), n_lazy, replace=False)

        clf = LazyClassifier(
            verbose=0,
            ignore_warnings=True,
            custom_metric=None,
            random_state=self.config["data"]["random_state"],
        )
        models, _ = clf.fit(X_train[idx], X_test, y_train[idx], y_test)

        print(f"\n  Top 5 modelos:")
        print(models.head(5).to_string())

        # Salvar CSV
        reports_path = self.config["paths"]["reports"]
        os.makedirs(reports_path, exist_ok=True)
        csv_path = os.path.join(reports_path, "benchmark_results.csv")
        models.to_csv(csv_path)
        print(f"  ✔ Resultados salvos: {csv_path}")

        self.plot_benchmark_ranking(models)

        print("=" * 60 + "\n")
        return models

    
    def plot_optuna_results(self, study):
        """
        Dois subplots matplotlib puro:
        1. Convergência: valor por trial + melhor acumulado (cummax)
        2. Importância dos hiperparâmetros: |correlação| com F1
        CORREÇÃO: sem optuna.visualization (Plotly) — sem kaleido.
        """
        print("  Gerando gráficos do Optuna (matplotlib puro)...")

        trials_df = study.trials_dataframe()
        param_cols = [c for c in trials_df.columns if c.startswith("params_")]

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

        # ── Subplot 1: Convergência ────────────────────────
        ax1.plot(trials_df["number"], trials_df["value"],
                 alpha=0.4, color="steelblue", linewidth=1, label="Trial")
        ax1.plot(trials_df["number"], trials_df["value"].cummax(),
                 color="darkred", linewidth=2.5, label="Melhor acumulado")
        ax1.set_xlabel("Trial")
        ax1.set_ylabel("F1-macro (CV)")
        ax1.set_title("Convergência do Optuna", fontweight="bold")
        ax1.legend()

        # ── Subplot 2: Importância dos hiperparâmetros ─────
        # Aproximação: |correlação de Pearson| de cada param com o valor do trial
        importances = {}
        for col in param_cols:
            try:
                r = pd.to_numeric(trials_df[col], errors="coerce").corr(trials_df["value"])
                if not np.isnan(r):
                    importances[col.replace("params_", "")] = abs(r)
            except Exception:
                pass

        if importances:
            imp_s = pd.Series(importances).sort_values()
            colors = plt.cm.RdYlGn(np.linspace(0.2, 0.9, len(imp_s)))
            bars = ax2.barh(imp_s.index, imp_s.values, color=colors, edgecolor="white")
            ax2.bar_label(bars, fmt="%.3f", padding=4)
            ax2.set_xlabel("|Correlação com F1-macro|")
            ax2.set_title("Importância dos Hiperparâmetros\n(correlação |r| com F1)",
                          fontweight="bold")
        else:
            ax2.text(0.5, 0.5, "Dados insuficientes\npara calcular importância",
                     ha="center", va="center", transform=ax2.transAxes)

        plt.suptitle(
            f"Resultados do Optuna — Melhor F1: {study.best_value:.4f}",
            fontsize=14, fontweight="bold",
        )
        plt.tight_layout()
        return self._save_fig(fig, "optuna_resultados.png")

    def optimize_hyperparameters(self, X_train, y_train):
        """
        Otimização com Optuna + StratifiedKFold.
        Espaço de busca: 6 hiperparâmetros.
        Métrica: F1-macro (penaliza classes minoritárias igualmente).
        """
        print("\n" + "=" * 60)
        print("    OTIMIZAÇÃO DE HIPERPARÂMETROS — Optuna")
        print("=" * 60)

        skf = StratifiedKFold(
            n_splits=self.config["optuna"].get("n_splits", 5),
            shuffle=True,
            random_state=self.config["data"]["random_state"],
        )

        def objective(trial):
            params = {
                "n_estimators":      trial.suggest_int("n_estimators", 100, 600, step=50),
                "max_depth":         trial.suggest_int("max_depth", 3, 30),
                "min_samples_split": trial.suggest_int("min_samples_split", 2, 20),
                "min_samples_leaf":  trial.suggest_int("min_samples_leaf", 1, 10),
                "max_features":      trial.suggest_categorical("max_features",
                                         ["sqrt", "log2", None]),
                "class_weight":      trial.suggest_categorical("class_weight",
                                         ["balanced", "balanced_subsample", None]),
            }
            clf    = RandomForestClassifier(
                **params,
                random_state=self.config["data"]["random_state"],
                n_jobs=-1,
            )
            scores = cross_val_score(
                clf, X_train, y_train,
                cv=skf, scoring="f1_macro", n_jobs=-1,
            )
            return scores.mean()

        study = optuna.create_study(
            direction="maximize",
            study_name="ambiental_rf",
        )
        study.optimize(
            objective,
            n_trials=self.config["optuna"]["n_trials"],
            show_progress_bar=True,
        )

        print(f"\n   Melhor F1-macro (CV): {study.best_value:.4f}")
        print("   Melhores hiperparâmetros:")
        for k, v in study.best_params.items():
            print(f"     {k}: {v}")

        self.plot_optuna_results(study)

        print("=" * 60 + "\n")
        return study.best_params, study.best_value

    
    def plot_confusion_matrix(self, y_true, y_pred, label_encoder):
        """
        Dois subplots: contagens absolutas e normalizada por linha.
        """
        print("  Gerando matrizes de confusão...")

        classes    = label_encoder.classes_
        cm_abs     = confusion_matrix(y_true, y_pred)
        cm_norm    = confusion_matrix(y_true, y_pred, normalize="true")

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

        ConfusionMatrixDisplay(cm_abs,  display_labels=classes).plot(
            cmap="Blues", ax=ax1, colorbar=False)
        ax1.set_title("Matriz de Confusão — Contagens", fontweight="bold")
        ax1.tick_params(axis="x", rotation=30)

        ConfusionMatrixDisplay(cm_norm.round(2), display_labels=classes).plot(
            cmap="Greens", ax=ax2, colorbar=False)
        ax2.set_title("Matriz de Confusão — Normalizada", fontweight="bold")
        ax2.tick_params(axis="x", rotation=30)

        plt.tight_layout()
        return self._save_fig(fig, "confusion_matrix.png")

    
    def plot_feature_importance(self, model, feature_names):
        """
        Barplot horizontal ordenado por importância (colormap RdYlGn).
        """
        print("  Gerando gráfico de importância das features...")

        if not hasattr(model, "feature_importances_"):
            print("   Modelo não possui feature_importances_.")
            return None

        imp    = pd.Series(model.feature_importances_, index=feature_names).sort_values()
        colors = plt.cm.RdYlGn(np.linspace(0.2, 0.9, len(imp)))

        fig, ax = plt.subplots(figsize=(10, 6))
        bars    = ax.barh(imp.index, imp.values, color=colors, edgecolor="white")
        ax.bar_label(bars, fmt="%.4f", padding=5)
        ax.set_title("Importância das Features — Random Forest",
                     fontweight="bold", fontsize=13)
        ax.set_xlabel("Importância (Gini)")
        plt.tight_layout()
        return self._save_fig(fig, "feature_importance.png")

    
    def plot_training_dashboard(self, y_test, y_pred, model,
                                 feature_names, label_encoder,
                                 metrics, study_best_value):
        """
        Painel escuro com 4 blocos:
        1. Métricas finais (barras)
        2. Matriz de confusão normalizada
        3. Importância das features
        4. Comparativo F1 Optuna CV vs teste final
        """
        print("  Gerando dashboard final de treinamento...")

        classes  = label_encoder.classes_
        cm_norm  = confusion_matrix(y_test, y_pred, normalize="true")
        imp      = pd.Series(model.feature_importances_, index=feature_names).sort_values()
        cores_cl = [CLASS_PALETTE.get(c, "#95a5a6") for c in classes]

        fig = plt.figure(figsize=(20, 14), facecolor="#1a1a2e")
        fig.suptitle(" Dashboard Final — Treinamento e Avaliação",
                     fontsize=17, fontweight="bold", color="white", y=0.98)
        gs = gridspec.GridSpec(2, 3, figure=fig, hspace=0.45, wspace=0.35)

        dark, border = "#16213e", "#444"

        def _style(ax, title):
            ax.set_facecolor(dark)
            ax.tick_params(colors="white", labelsize=8)
            for sp in ax.spines.values():
                sp.set_color(border)
            ax.set_title(title, color="white", fontweight="bold", fontsize=10)

        # ── Painel 1: Métricas ───────────────────────────
        ax1 = fig.add_subplot(gs[0, 0])
        m_names = ["Accuracy", "Precision\n(macro)", "Recall\n(macro)", "F1\n(macro)"]
        m_vals  = [metrics["accuracy"], metrics["precision_macro"],
                   metrics["recall_macro"], metrics["f1_macro"]]
        bar_c   = ["#3498db", "#2ecc71", "#e67e22", "#9b59b6"]
        bars    = ax1.barh(m_names, m_vals, color=bar_c, edgecolor="white")
        ax1.bar_label(bars, fmt="%.4f", padding=4, color="white", fontsize=9)
        ax1.set_xlim(0, 1.15)
        _style(ax1, "Métricas Finais")
        ax1.set_xlabel("Valor", color="white", fontsize=8)

        # ── Painel 2: Matriz normalizada ─────────────────
        ax2 = fig.add_subplot(gs[0, 1:])
        im = ax2.imshow(cm_norm, cmap="Blues", vmin=0, vmax=1)
        ax2.set_xticks(range(len(classes)))
        ax2.set_xticklabels(classes, rotation=30, color="white", fontsize=8)
        ax2.set_yticks(range(len(classes)))
        ax2.set_yticklabels(classes, color="white", fontsize=8)
        for i in range(len(classes)):
            for j in range(len(classes)):
                ax2.text(j, i, f"{cm_norm[i,j]:.2f}", ha="center", va="center",
                         color="black" if cm_norm[i, j] > 0.5 else "white", fontsize=8)
        _style(ax2, "Matriz de Confusão (normalizada por linha)")
        plt.colorbar(im, ax=ax2, shrink=0.8)

        # ── Painel 3: Feature importance ─────────────────
        ax3 = fig.add_subplot(gs[1, :2])
        colors_imp = plt.cm.RdYlGn(np.linspace(0.2, 0.9, len(imp)))
        bars3 = ax3.barh(imp.index, imp.values, color=colors_imp, edgecolor="white")
        ax3.bar_label(bars3, fmt="%.4f", padding=4, color="white", fontsize=8)
        _style(ax3, "Importância das Features (Gini)")
        ax3.set_xlabel("Importância", color="white", fontsize=8)

        # ── Painel 4: Comparativo CV vs Teste ────────────
        ax4 = fig.add_subplot(gs[1, 2])
        comp_labels = ["F1 Optuna\n(CV treino)", "F1 Final\n(teste)"]
        comp_vals   = [study_best_value, metrics["f1_macro"]]
        comp_colors = ["#e67e22", "#2ecc71"]
        bars4 = ax4.bar(comp_labels, comp_vals, color=comp_colors, edgecolor="white", width=0.5)
        ax4.bar_label(bars4, fmt="%.4f", padding=4, color="white", fontsize=10)
        ax4.set_ylim(0, 1.15)
        ax4.axhline(1.0, color="white", linestyle="--", alpha=0.3)
        _style(ax4, "F1-macro: Optuna CV vs Teste Final")
        ax4.set_ylabel("F1-macro", color="white", fontsize=8)

        filepath = os.path.join(self.figures_dir, "training_dashboard.png")
        fig.savefig(filepath, bbox_inches="tight")
        plt.close(fig)
        print(f"   Dashboard salvo: {filepath}")
        return filepath

    
    def verify_saved_artifacts(self, feature_names):
        """
        Carrega modelo, pipeline e encoder salvos em disco.
        Roda inferência em amostra sintética com valores médios.
        Imprime predição e probabilidades com barra ASCII.
        """
        print("\n   Verificando artefatos salvos...")
        models_path  = self.config["paths"]["models"]
        pipeline_path = os.path.join(models_path, "preprocessing_pipeline.joblib")
        encoder_path  = os.path.join(models_path, "label_encoder.joblib")
        model_path    = os.path.join(models_path, "best_model.joblib")

        try:
            loaded_model    = joblib.load(model_path)
            loaded_pipeline = joblib.load(pipeline_path)
            loaded_encoder  = joblib.load(encoder_path)

            # Amostra sintética com valores médios do scaler
            scaler_means = loaded_pipeline.named_steps["scaler"].mean_
            synthetic_df = pd.DataFrame(
                [scaler_means], columns=feature_names
            )

            X_trans   = loaded_pipeline.transform(synthetic_df)
            pred_enc  = loaded_model.predict(X_trans)
            pred_proba = loaded_model.predict_proba(X_trans)[0]
            pred_label = loaded_encoder.inverse_transform(pred_enc)[0]

            col_w = max(len(c) for c in loaded_encoder.classes_) + 2
            print("\n  ┌─ Smoke Test de Inferência " + "─" * 25)
            print(f"  │  Modelo   : {type(loaded_model).__name__}")
            print(f"  │  Pipeline : {type(loaded_pipeline).__name__}")
            print(f"  │  Encoder  : {type(loaded_encoder).__name__}")
            print(f"  │  Predição : {pred_label}")
            print("  │  Probabilidades:")
            for cls, prob in zip(loaded_encoder.classes_, pred_proba):
                bar = "█" * int(prob * 30)
                print(f"  │    {cls:<{col_w}} {bar:<30} {prob:.3f}")
            print("  └" + "─" * 53)
            return True

        except Exception as e:
            print(f"   Erro na verificação de artefatos: {e}")
            return False

    # ─────────────────────────────────────────────────────────
    # TREINAMENTO FINAL + MLFLOW
    # ─────────────────────────────────────────────────────────
    def train_final_model(self, X_train, X_test, y_train, y_test,
                           best_params, study_best_value,
                           label_encoder, feature_names):
        """
        Treina modelo final com melhores hiperparâmetros.
        Registra parâmetros, métricas, modelo e artefatos no MLflow.

        Retorna
        -------
        model   : RandomForestClassifier treinado
        metrics : dict com accuracy, precision_macro, recall_macro, f1_macro
        y_pred  : np.ndarray com predições no teste
        """
        print("\n" + "=" * 60)
        print("    TREINAMENTO FINAL + MLFLOW")
        print("=" * 60)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        run_name  = f"RF_Optuna_{timestamp}"

        with mlflow.start_run(run_name=run_name):

            # ── Tags ──────────────────────────────────────
            mlflow.set_tags({
                "model_type":  "RandomForestClassifier",
                "optimizer":   "Optuna",
                "balancing":   "SMOTE",
                "timestamp":   timestamp,
                "n_features":  len(feature_names),
                "n_classes":   len(label_encoder.classes_),
            })

            # ── Parâmetros ────────────────────────────────
            mlflow.log_params(best_params)
            mlflow.log_param("random_state",   self.config["data"]["random_state"])
            mlflow.log_param("test_size",      self.config["data"]["test_size"])
            mlflow.log_param("optuna_trials",  self.config["optuna"]["n_trials"])
            mlflow.log_param("optuna_best_cv", round(study_best_value, 4))

            # ── Treinamento ───────────────────────────────
            print("\n  Treinando RandomForest com melhores hiperparâmetros...")
            model = RandomForestClassifier(
                **best_params,
                random_state=self.config["data"]["random_state"],
                n_jobs=-1,
            )
            model.fit(X_train, y_train)
            y_pred = model.predict(X_test)

            # ── Métricas ──────────────────────────────────
            metrics = {
                "accuracy":        accuracy_score(y_test, y_pred),
                "precision_macro": precision_score(y_test, y_pred,
                                                   average="macro", zero_division=0),
                "recall_macro":    recall_score(y_test, y_pred,
                                               average="macro", zero_division=0),
                "f1_macro":        f1_score(y_test, y_pred,
                                           average="macro", zero_division=0),
            }
            mlflow.log_metrics(metrics)

            print("\n   Métricas do modelo final:")
            col_w = 20
            for k, v in metrics.items():
                print(f"    {k:<{col_w}}: {v:.4f}")

            print("\n   Classification Report:")
            print(classification_report(
                y_test, y_pred,
                target_names=label_encoder.classes_,
                zero_division=0,
            ))

            # ── Salvar modelo ─────────────────────────────
            models_path = self.config["paths"]["models"]
            os.makedirs(models_path, exist_ok=True)
            model_file  = os.path.join(models_path, "best_model.joblib")
            joblib.dump(model, model_file)
            mlflow.sklearn.log_model(model, "random_forest_model")
            print(f"   Modelo salvo: {model_file}")

            # ── Gráficos e artefatos ──────────────────────
            cm_path   = self.plot_confusion_matrix(y_test, y_pred, label_encoder)
            imp_path  = self.plot_feature_importance(model, feature_names)
            dash_path = self.plot_training_dashboard(
                y_test, y_pred, model,
                feature_names, label_encoder,
                metrics, study_best_value,
            )

            for path in [cm_path, imp_path, dash_path]:
                if path and os.path.exists(path):
                    mlflow.log_artifact(path)

            # ── Resumo em texto ───────────────────────────
            summary_path = os.path.join(
                self.config["paths"]["reports"], "resumo_experimento.txt"
            )
            with open(summary_path, "w", encoding="utf-8") as f:
                f.write("=" * 55 + "\n")
                f.write("  RESUMO DO EXPERIMENTO — Qualidade Ambiental\n")
                f.write("=" * 55 + "\n\n")
                f.write(f"Timestamp    : {timestamp}\n")
                f.write(f"Run name     : {run_name}\n")
                f.write(f"Features     : {feature_names}\n")
                f.write(f"Classes      : {list(label_encoder.classes_)}\n\n")
                f.write("MELHORES HIPERPARÂMETROS (Optuna):\n")
                for k, v in best_params.items():
                    f.write(f"  {k}: {v}\n")
                f.write(f"\nF1-macro CV (Optuna) : {study_best_value:.4f}\n\n")
                f.write("MÉTRICAS NO TESTE:\n")
                for k, v in metrics.items():
                    f.write(f"  {k}: {v:.4f}\n")
                f.write("\nCLASSIFICATION REPORT:\n")
                f.write(classification_report(
                    y_test, y_pred,
                    target_names=label_encoder.classes_,
                    zero_division=0,
                ))
            mlflow.log_artifact(summary_path)
            print(f"  Resumo salvo: {summary_path}")

            run_id = mlflow.active_run().info.run_id
            print(f"\n   MLflow run registrado: {run_name} | ID: {run_id}")

        # ── Verificação final ─────────────────────────────
        self.verify_saved_artifacts(feature_names)

        print("\n" + "=" * 60)
        print("    TREINAMENTO CONCLUÍDO")
        print("=" * 60 + "\n")

        return model, metrics, y_pred



if __name__ == "__main__":
    import yaml
    from preprocessing import DataPreprocessor

    with open("../config.yaml", "r") as f:
        config = yaml.safe_load(f)

    import pandas as pd
    df = pd.read_csv("../data/dataset_ambiental.csv")

    preprocessor = DataPreprocessor(config)
    X_train, X_test, y_train, y_test, feature_names, stats = preprocessor.prepare_data(df)

    trainer = ModelTrainer(config)

    # Benchmark
    trainer.run_benchmark(X_train, X_test, y_train, y_test)

    # Otimização
    best_params, best_cv = trainer.optimize_hyperparameters(X_train, y_train)

    # Treinamento final
    model, metrics, y_pred = trainer.train_final_model(
        X_train, X_test, y_train, y_test,
        best_params, best_cv,
        preprocessor.label_encoder,
        feature_names,
    )