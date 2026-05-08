import os
import sys
import yaml
import pandas as pd

from src.eda import run_eda
from src.preprocessing import DataPreprocessor
from src.training import ModelTrainer


# ─────────────────────────────────────────────────────────────────
# UTILITÁRIOS
# ─────────────────────────────────────────────────────────────────
def _banner(title: str, width: int = 60) -> None:
    """Imprime um banner de seção padronizado."""
    print("\n" + "=" * width)
    print(f"   {title}")
    print("=" * width)


def _load_config(path: str = "config.yaml") -> dict:
    """Carrega e valida o arquivo de configuração."""
    if not os.path.exists(path):
        print(f" Erro: '{path}' não encontrado.")
        sys.exit(1)

    with open(path, "r", encoding="utf-8") as f:
        config = yaml.safe_load(f)

    # Validação mínima de chaves obrigatórias
    required = {
        "data":    ["raw_path", "target", "test_size", "random_state"],
        "paths":   ["reports", "models"],
        "features":["numerical"],
        "optuna":  ["n_trials"],
        "mlflow":  ["tracking_uri", "experiment_name"],
    }
    for section, keys in required.items():
        for key in keys:
            if key not in config.get(section, {}):
                print(f" config.yaml: chave obrigatória '{section}.{key}' ausente.")
                sys.exit(1)

    return config


def _create_dirs(config: dict) -> str:
    """Cria todos os diretórios necessários e retorna o caminho de figuras."""
    for path in config["paths"].values():
        os.makedirs(path, exist_ok=True)

    figures_dir = os.path.join(config["paths"]["reports"], "figures")
    os.makedirs(figures_dir, exist_ok=True)
    return figures_dir


def _load_dataset(config: dict) -> pd.DataFrame:
    """Carrega o CSV bruto e exibe diagnóstico inicial."""
    raw_path = config["data"]["raw_path"]
    if not os.path.exists(raw_path):
        print(f" Dataset não encontrado: '{raw_path}'")
        sys.exit(1)

    df = pd.read_csv(raw_path)
    print(f"   Dataset carregado: {df.shape[0]:,} linhas × {df.shape[1]} colunas")
    print(f"   Colunas: {df.columns.tolist()}")

    # Verificação rápida do target
    target = config["data"]["target"]
    if target not in df.columns:
        print(f" Coluna target '{target}' não encontrada no dataset.")
        sys.exit(1)

    print(f"   Classes no target '{target}': "
          f"{sorted(df[target].dropna().unique().tolist())}")
    return df


# ─────────────────────────────────────────────────────────────────
# ETAPAS DO PIPELINE
# ─────────────────────────────────────────────────────────────────
def step_eda(df: pd.DataFrame, config: dict) -> dict:
    """
    Etapa 1 — EDA completa.
    IMPORTANTE: a EDA recebe o target como string (sem encoding).
    A ordem das classes é lida do config ou inferida do DataFrame.
    """
    _banner("ETAPA 1 — ANÁLISE EXPLORATÓRIA (EDA)")

    target     = config["data"]["target"]
    features   = config["features"]["numerical"]

    # Ordem das classes: preferência ao config, fallback para sorted únicos
    ordem_classes = config["data"].get(
        "class_order",
        sorted(df[target].dropna().unique().tolist()),
    )

    output_path = os.path.join(config["paths"]["reports"], "eda_report.html")

    eda_paths = run_eda(
        df=df,
        output_path=output_path,
        features=features,
        target_col=target,
        ordem_classes=ordem_classes,
    )

    print(f"\n  {len(eda_paths)} artefatos de EDA gerados.")
    return eda_paths


def step_preprocessing(df: pd.DataFrame, config: dict):
    """
    Etapa 2 — Pré-processamento completo.
    Retorna dados prontos para treino e o preprocessor (com label_encoder).
    """
    _banner("ETAPA 2 — PRÉ-PROCESSAMENTO")

    preprocessor = DataPreprocessor(config)
    X_train, X_test, y_train, y_test, feature_names, stats = \
        preprocessor.prepare_data(df)

    # Atualizar config com as features reais pós-processamento
    # (podem diferir se colunas foram removidas)
    config["features"]["numerical"] = feature_names

    return preprocessor, X_train, X_test, y_train, y_test, feature_names


def step_benchmark(trainer: ModelTrainer,
                   X_train, X_test, y_train, y_test) -> pd.DataFrame:
    """
    Etapa 3 — Benchmark com LazyPredict.
    Já usa numpy arrays (preprocessing retorna numpy).
    """
    _banner("ETAPA 3 — BENCHMARK (LazyPredict)")

    # Garantir numpy (defensivo: preprocessing já retorna ndarray)
    X_tr = X_train if not isinstance(X_train, pd.DataFrame) else X_train.to_numpy()
    X_te = X_test  if not isinstance(X_test,  pd.DataFrame) else X_test.to_numpy()

    return trainer.run_benchmark(X_tr, X_te, y_train, y_test)


def step_optimize(trainer: ModelTrainer,
                  X_train, y_train) -> tuple:
    """
    Etapa 4 — Otimização de hiperparâmetros com Optuna.
    Retorna (best_params, best_cv_f1).
    CORREÇÃO: X_test/y_test removidos — Optuna usa apenas CV no treino.
    """
    _banner("ETAPA 4 — OTIMIZAÇÃO (Optuna)")

    X_tr = X_train if not isinstance(X_train, pd.DataFrame) else X_train.to_numpy()
    return trainer.optimize_hyperparameters(X_tr, y_train)


def step_train(trainer: ModelTrainer,
               X_train, X_test, y_train, y_test,
               best_params: dict, best_cv: float,
               preprocessor: DataPreprocessor,
               feature_names: list) -> tuple:
    """
    Etapa 5 — Treinamento final, avaliação e registro no MLflow.
    Retorna (model, metrics, y_pred).
    """
    _banner("ETAPA 5 — TREINAMENTO FINAL + MLFLOW")

    X_tr = X_train if not isinstance(X_train, pd.DataFrame) else X_train.to_numpy()
    X_te = X_test  if not isinstance(X_test,  pd.DataFrame) else X_test.to_numpy()

    return trainer.train_final_model(
        X_train=X_tr,
        X_test=X_te,
        y_train=y_train,
        y_test=y_test,
        best_params=best_params,
        study_best_value=best_cv,
        label_encoder=preprocessor.label_encoder,
        feature_names=feature_names,
    )


# ─────────────────────────────────────────────────────────────────
# SUMÁRIO FINAL
# ─────────────────────────────────────────────────────────────────
def _print_summary(metrics: dict, config: dict) -> None:
    """Imprime o sumário final do pipeline."""
    _banner(" PIPELINE CONCLUÍDO", width=60)

    col_w = 22
    print(f"\n  {'Accuracy':<{col_w}}: {metrics['accuracy']:.4f}  "
          f"({metrics['accuracy']*100:.2f}%)")
    print(f"  {'Precision (macro)':<{col_w}}: {metrics['precision_macro']:.4f}")
    print(f"  {'Recall (macro)':<{col_w}}: {metrics['recall_macro']:.4f}")
    print(f"  {'F1-score (macro)':<{col_w}}: {metrics['f1_macro']:.4f}")

    print(f"\n  Artefatos em  : {config['paths']['models']}/")
    print(f"  Relatórios em : {config['paths']['reports']}/")
    print(f"  MLflow UI     : mlflow ui  →  {config['mlflow']['tracking_uri']}")
    print("\n" + "=" * 60 + "\n")


# ─────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────
def main() -> None:
    _banner(" PIPELINE — QUALIDADE AMBIENTAL", width=60)

    # ── Configuração ──────────────────────────────────────────
    config = _load_config("config.yaml")
    _create_dirs(config)

    # ── Dados ─────────────────────────────────────────────────
    _banner("ETAPA 0 — CARREGAMENTO DOS DADOS")
    df = _load_dataset(config)

    # ── EDA ───────────────────────────────────────────────────
    step_eda(df, config)

    # ── Pré-processamento ─────────────────────────────────────
    preprocessor, X_train, X_test, y_train, y_test, feature_names = \
        step_preprocessing(df, config)

    # ── Trainer (instância única compartilhada entre etapas) ──
    trainer = ModelTrainer(config)

    # ── Benchmark ─────────────────────────────────────────────
    step_benchmark(trainer, X_train, X_test, y_train, y_test)

    # ── Otimização ────────────────────────────────────────────
    best_params, best_cv = step_optimize(trainer, X_train, y_train)

    # ── Treinamento Final ─────────────────────────────────────
    model, metrics, y_pred = step_train(
        trainer, X_train, X_test, y_train, y_test,
        best_params, best_cv,
        preprocessor, feature_names,
    )

    # ── Sumário ───────────────────────────────────────────────
    _print_summary(metrics, config)


if __name__ == "__main__":
    main()