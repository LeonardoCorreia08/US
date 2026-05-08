import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import seaborn as sns
from ydata_profiling import ProfileReport
import os

# ─────────────────────────────────────────────────────────────────
# CONFIGURAÇÕES GLOBAIS
# ─────────────────────────────────────────────────────────────────
RANDOM_STATE = 42
sns.set_style("whitegrid")
plt.rcParams.update({
    "figure.dpi": 130,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "font.family": "DejaVu Sans",
})

# Paleta de cores por classe — centralizada para consistência em todos os plots
CLASS_PALETTE = {
    "Excelente":  "#2ecc71",
    "Boa":        "#27ae60",
    "Moderada":   "#f39c12",
    "Ruim":       "#e74c3c",
    "Muito Ruim": "#8e44ad",
}


# ─────────────────────────────────────────────────────────────────
# UTILITÁRIO DE SALVAMENTO
# ─────────────────────────────────────────────────────────────────
def _save_plot(fig, filename, output_dir):
    """Salva uma figura matplotlib em output_dir e fecha a figura."""
    os.makedirs(output_dir, exist_ok=True)
    filepath = os.path.join(output_dir, filename)
    fig.savefig(filepath, bbox_inches="tight")
    plt.close(fig)
    print(f"  ✔ Gráfico salvo: {filepath}")
    return filepath


# ─────────────────────────────────────────────────────────────────
# 1. DISTRIBUIÇÕES UNIVARIADAS
# Correção: KDE em eixo twin para não distorcer escala do histograma
# ─────────────────────────────────────────────────────────────────
def plot_distributions(df, features, output_dir):
    """
    Histograma + KDE (eixo twin) para cada feature numérica.
    Linhas verticais de média (vermelho) e mediana (verde).
    """
    print("Gerando gráficos de distribuição...")
    numeric_features = [f for f in features if pd.api.types.is_numeric_dtype(df[f])]
    if not numeric_features:
        print("  Nenhuma feature numérica encontrada.")
        return {}

    n_cols = 4
    n_rows = (len(numeric_features) + n_cols - 1) // n_cols
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(n_cols * 5, n_rows * 4))
    axes = axes.flatten()

    for i, feature in enumerate(numeric_features):
        data = df[feature].dropna()
        ax = axes[i]

        # Histograma no eixo principal
        ax.hist(data, bins=40, color="steelblue", alpha=0.75, edgecolor="white")
        ax.set_title(feature, fontweight="bold")
        ax.set_xlabel("Valor")
        ax.set_ylabel("Frequência")

        # KDE em eixo twin — escala independente
        ax_kde = ax.twinx()
        sns.kdeplot(data, ax=ax_kde, color="darkred", linewidth=2)
        ax_kde.set_ylabel("")
        ax_kde.set_yticks([])
        # Remover espinhos do eixo twin para não poluir
        for spine in ["top", "right"]:
            ax_kde.spines[spine].set_visible(False)

        # Linhas de média e mediana
        ax.axvline(data.mean(),   color="red",   linestyle="--", linewidth=1.5,
                   label=f"μ = {data.mean():.2f}")
        ax.axvline(data.median(), color="green", linestyle="-",  linewidth=1.5,
                   label=f"med = {data.median():.2f}")
        ax.legend(fontsize=8)

    # Ocultar subplots vazios
    for j in range(len(numeric_features), len(axes)):
        axes[j].set_visible(False)

    plt.suptitle("Distribuição das Variáveis (Histograma + KDE)", fontsize=15,
                 fontweight="bold", y=1.01)
    plt.tight_layout()
    path = _save_plot(fig, "distribuicoes.png", output_dir)
    return {"distribuicoes": path}


# ─────────────────────────────────────────────────────────────────
# 2. BALANCEAMENTO DO TARGET
# ─────────────────────────────────────────────────────────────────
def plot_target_balance(df, target_col, ordem_classes, output_dir):
    """
    Barras horizontais com contagem + pizza com percentual.
    Calcula e imprime ratio majority/minority.
    """
    print("Gerando gráficos de balanceamento do target...")

    counts      = df[target_col].value_counts().reindex(ordem_classes, fill_value=0)
    percentages = df[target_col].value_counts(normalize=True).reindex(ordem_classes, fill_value=0)
    cores       = [CLASS_PALETTE.get(c, "#95a5a6") for c in ordem_classes]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Barras horizontais
    bars = ax1.barh(counts.index, counts.values, color=cores, edgecolor="white")
    ax1.bar_label(bars, fmt="%d", padding=6)
    ax1.set_title(f"Contagem por Classe — {target_col}", fontweight="bold")
    ax1.set_xlabel("Nº de amostras")
    ax1.invert_yaxis()

    # Pizza
    ax2.pie(
        percentages.values,
        labels=percentages.index,
        autopct="%1.1f%%",
        startangle=140,
        colors=cores,
        pctdistance=0.80,
    )
    ax2.set_title(f"Proporção das Classes — {target_col}", fontweight="bold")

    # Ratio
    if counts.min() > 0:
        ratio = counts.max() / counts.min()
        print(f"  ⚠ Ratio Majority/Minority: {ratio:.1f}x  "
              f"({counts.idxmax()} vs {counts.idxmin()})")
    else:
        print("  ⚠ Classe minoritária com 0 amostras após reindex.")

    plt.suptitle("Distribuição do Target — Desbalanceamento de Classes",
                 fontsize=14, fontweight="bold")
    plt.tight_layout()
    path = _save_plot(fig, "target_distribuicao.png", output_dir)
    return {"target_distribuicao": path}


# ─────────────────────────────────────────────────────────────────
# 3. MATRIZ DE CORRELAÇÃO
# Correção: filtro de pares duplicados usando máscara triangular
# ─────────────────────────────────────────────────────────────────
def plot_correlation_matrix(df, features, output_dir):
    """
    Heatmap Pearson com máscara triangular superior.
    Imprime pares com |r| > 0.5 sem duplicatas.
    """
    print("Gerando matriz de correlação...")

    numeric_df = df[features].apply(pd.to_numeric, errors="coerce")
    if numeric_df.empty:
        print("  Sem features numéricas.")
        return {}

    corr = numeric_df.corr(method="pearson")
    mask = np.triu(np.ones_like(corr, dtype=bool))  # mascara o triângulo superior

    fig, ax = plt.subplots(figsize=(12, 10))
    sns.heatmap(
        corr, mask=mask, annot=True, fmt=".2f",
        cmap="RdBu_r", center=0, vmin=-1, vmax=1,
        linewidths=0.5, ax=ax, cbar_kws={"shrink": 0.8},
        annot_kws={"size": 10},
    )
    ax.set_title("Matriz de Correlação (Pearson)", fontweight="bold", fontsize=14)
    plt.xticks(rotation=45, ha="right")
    plt.yticks(rotation=0)
    plt.tight_layout()
    path = _save_plot(fig, "correlacao.png", output_dir)

    # Pares com |r| > 0.5 — usando a máscara para evitar duplicatas e auto-correlações
    print("  Pares com correlação |r| > 0.5:")
    already_seen = set()
    found_any = False
    for col_a in corr.columns:
        for col_b in corr.columns:
            if col_a == col_b:
                continue
            pair = tuple(sorted([col_a, col_b]))
            if pair in already_seen:
                continue
            already_seen.add(pair)
            r = corr.loc[col_a, col_b]
            if abs(r) > 0.5:
                print(f"    {col_a} × {col_b}: r = {r:.3f}")
                found_any = True
    if not found_any:
        print("    Nenhum par com |r| > 0.5 encontrado.")

    return {"correlacao": path}


# ─────────────────────────────────────────────────────────────────
# 4. BOXPLOTS POR CLASSE
# ─────────────────────────────────────────────────────────────────
def plot_boxplots_by_class(df, features, target_col, ordem_classes, output_dir):
    """
    Boxplot de cada feature numérica separado por classe do target.
    """
    print("Gerando boxplots por classe...")

    numeric_features = [f for f in features if pd.api.types.is_numeric_dtype(df[f])]
    if not numeric_features:
        return {}

    cores = [CLASS_PALETTE.get(c, "#95a5a6") for c in ordem_classes]
    palette = dict(zip(ordem_classes, cores))

    n_cols = 4
    n_rows = (len(numeric_features) + n_cols - 1) // n_cols
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(n_cols * 5, n_rows * 4))
    axes = axes.flatten()

    for i, feature in enumerate(numeric_features):
        sns.boxplot(
            data=df, x=target_col, y=feature,
            order=ordem_classes, palette=palette,
            ax=axes[i],
            flierprops={"markersize": 3, "alpha": 0.4},
        )
        axes[i].set_title(feature, fontweight="bold")
        axes[i].set_xlabel("")
        axes[i].tick_params(axis="x", rotation=30)

    for j in range(len(numeric_features), len(axes)):
        axes[j].set_visible(False)

    plt.suptitle("Distribuição das Features por Classe (com Outliers)",
                 fontsize=15, fontweight="bold", y=1.01)
    plt.tight_layout()
    path = _save_plot(fig, "boxplots_classes.png", output_dir)
    return {"boxplots_classes": path}


# ─────────────────────────────────────────────────────────────────
# 5. MAPA DE NULOS
# ─────────────────────────────────────────────────────────────────
def plot_null_map(df, output_dir):
    """
    Heatmap binário de nulos + tabela com % por coluna.
    Retorna caminhos mesmo quando não há nulos (tabela ainda é impressa).
    """
    print("Gerando mapa de nulos...")

    null_counts = df.isnull().sum()
    null_pct    = (null_counts / len(df) * 100).round(2)
    null_info   = pd.DataFrame({"Nulos": null_counts, "% Nulos": null_pct})
    null_info   = null_info[null_info["Nulos"] > 0].sort_values("% Nulos", ascending=False)

    print("  Tabela de Nulos por Coluna:")
    if null_info.empty:
        print("    ✔ Nenhum valor nulo encontrado no dataset.")
        return {}
    else:
        print(null_info.to_string())

    fig, ax = plt.subplots(figsize=(14, 5))
    # Transposto: colunas no eixo Y, amostras no eixo X
    sns.heatmap(
        df.isnull().T.astype(int),
        cbar=False, cmap="Reds",
        ax=ax, yticklabels=df.columns,
    )
    ax.set_title("Mapa de Valores Nulos (vermelho = nulo)", fontweight="bold")
    ax.set_xlabel("Índice da linha")
    plt.tight_layout()
    path = _save_plot(fig, "mapa_nulos.png", output_dir)
    return {"mapa_nulos": path}


# ─────────────────────────────────────────────────────────────────
# 6. DETECÇÃO DE OUTLIERS — IQR
# ─────────────────────────────────────────────────────────────────
def detect_outliers_iqr(df, features):
    """
    Calcula Q1, Q3, IQR e nº de outliers por feature.
    Retorna DataFrame com o relatório completo.
    """
    print("Detectando outliers por IQR...")
    report = []

    for feature in features:
        if not pd.api.types.is_numeric_dtype(df[feature]):
            continue
        col = df[feature].dropna()
        Q1, Q3  = col.quantile(0.25), col.quantile(0.75)
        IQR     = Q3 - Q1
        lb, ub  = Q1 - 1.5 * IQR, Q3 + 1.5 * IQR
        n_out   = int(((col < lb) | (col > ub)).sum())

        report.append({
            "Feature":    feature,
            "Q1":         round(Q1, 4),
            "Q3":         round(Q3, 4),
            "IQR":        round(IQR, 4),
            "Lim. Inf":   round(lb, 4),
            "Lim. Sup":   round(ub, 4),
            "Outliers":   n_out,
            "% Outliers": round(n_out / len(col) * 100, 2),
        })

    report_df = pd.DataFrame(report)
    if not report_df.empty:
        print("\n  Relatório de Outliers (IQR):")
        print(report_df.to_string(index=False))
    else:
        print("  Nenhuma feature numérica encontrada.")

    return report_df


# ─────────────────────────────────────────────────────────────────
# 7. PAIRPLOT
# Correção: target_col com strings originais (não inteiros codificados)
# ─────────────────────────────────────────────────────────────────
def plot_pairplot(df, features_selecionadas, target_col, ordem_classes, output_dir,
                  sample_n=500, random_state=RANDOM_STATE):
    """
    Pairplot das features selecionadas coloridas pelo target.
    Usa amostra de sample_n linhas para não travar.
    IMPORTANTE: df deve conter o target como string (não codificado como inteiro).
    """
    print("Gerando pairplot (amostra)...")

    numeric_feats = [f for f in features_selecionadas
                     if f in df.columns and pd.api.types.is_numeric_dtype(df[f])]
    if len(numeric_feats) < 2:
        print("  Features insuficientes para pairplot.")
        return {}

    # Validar que o target existe e é string (não inteiro)
    if target_col not in df.columns:
        print(f"  Coluna '{target_col}' não encontrada no DataFrame.")
        return {}

    # Amostragem
    df_sample = df.sample(n=min(sample_n, len(df)), random_state=random_state)

    # Selecionar colunas: features numéricas + target
    cols = numeric_feats + [target_col]
    temp_df = df_sample[cols].copy()

    # Garantir que o target é Categorical com a ordem correta
    # Filtrar apenas classes que realmente aparecem na amostra
    classes_presentes = [c for c in ordem_classes if c in temp_df[target_col].values]
    temp_df[target_col] = pd.Categorical(
        temp_df[target_col],
        categories=classes_presentes,
        ordered=True,
    )

    palette = {c: CLASS_PALETTE.get(c, "#95a5a6") for c in classes_presentes}

    pp = sns.pairplot(
        temp_df,
        hue=target_col,
        hue_order=classes_presentes,
        palette=palette,
        diag_kind="kde",
        plot_kws={"alpha": 0.5, "s": 20},
    )
    pp.fig.suptitle(
        f"Pairplot — Features Principais × {target_col} (n={len(temp_df)})",
        y=1.02, fontweight="bold",
    )

    os.makedirs(output_dir, exist_ok=True)
    path = os.path.join(output_dir, "pairplot.png")
    pp.fig.savefig(path, bbox_inches="tight")
    plt.close(pp.fig)
    print(f"  ✔ Gráfico salvo: {path}")
    return {"pairplot": path}


# ─────────────────────────────────────────────────────────────────
# 8. DASHBOARD FINAL CONSOLIDADO
# ─────────────────────────────────────────────────────────────────
def plot_eda_dashboard(df, features, target_col, ordem_classes, output_dir):
    """
    Painel único consolidando os principais resultados da EDA:
    distribuição do target, correlação, nulos e outliers.
    """
    print("Gerando dashboard consolidado da EDA...")

    numeric_features = [f for f in features if pd.api.types.is_numeric_dtype(df[f])]
    cores = [CLASS_PALETTE.get(c, "#95a5a6") for c in ordem_classes]

    fig = plt.figure(figsize=(20, 14), facecolor="#1a1a2e")
    fig.suptitle(
        " Dashboard EDA — Dataset Ambiental",
        fontsize=18, fontweight="bold", color="white", y=0.98,
    )

    gs = gridspec.GridSpec(3, 3, figure=fig, hspace=0.45, wspace=0.35)

    # ── Painel 1: Contagem do target ───────────────────────────
    ax1 = fig.add_subplot(gs[0, 0])
    counts = df[target_col].value_counts().reindex(ordem_classes, fill_value=0)
    bars = ax1.barh(counts.index, counts.values, color=cores, edgecolor="white")
    ax1.bar_label(bars, fmt="%d", padding=4, color="white", fontsize=8)
    ax1.set_facecolor("#16213e")
    ax1.tick_params(colors="white", labelsize=8)
    for spine in ax1.spines.values(): spine.set_color("#444")
    ax1.set_title("Distribuição do Target", color="white", fontweight="bold", fontsize=10)
    ax1.invert_yaxis()

    # ── Painel 2: Pizza de proporções ─────────────────────────
    ax2 = fig.add_subplot(gs[0, 1])
    pcts = counts / counts.sum()
    ax2.pie(pcts.values, labels=pcts.index, autopct="%1.1f%%",
            colors=cores, startangle=140, pctdistance=0.8,
            textprops={"color": "white", "fontsize": 8})
    ax2.set_facecolor("#16213e")
    ax2.set_title("Proporção das Classes", color="white", fontweight="bold", fontsize=10)

    # ── Painel 3: Tabela de nulos ──────────────────────────────
    ax3 = fig.add_subplot(gs[0, 2])
    ax3.axis("off")
    ax3.set_facecolor("#16213e")
    null_pct = (df[features].isnull().sum() / len(df) * 100).round(2)
    null_pct = null_pct[null_pct > 0]
    if null_pct.empty:
        ax3.text(0.5, 0.5, " Sem valores nulos", ha="center", va="center",
                 color="#2ecc71", fontsize=12, transform=ax3.transAxes)
    else:
        table_data = [[f, f"{v:.2f}%"] for f, v in null_pct.items()]
        tbl = ax3.table(cellText=table_data, colLabels=["Feature", "% Nulos"],
                        cellLoc="center", loc="center")
        tbl.auto_set_font_size(False)
        tbl.set_fontsize(9)
        for (r, c), cell in tbl.get_celld().items():
            cell.set_facecolor("#16213e" if r > 0 else "#0f3460")
            cell.set_text_props(color="white")
            cell.set_edgecolor("#444")
    ax3.set_title("Valores Nulos", color="white", fontweight="bold", fontsize=10)

    # ── Painel 4: Heatmap de correlação ───────────────────────
    ax4 = fig.add_subplot(gs[1, :2])
    num_df = df[numeric_features].apply(pd.to_numeric, errors="coerce")
    corr   = num_df.corr()
    mask   = np.triu(np.ones_like(corr, dtype=bool))
    sns.heatmap(corr, mask=mask, annot=True, fmt=".2f", cmap="RdBu_r",
                center=0, vmin=-1, vmax=1, ax=ax4,
                annot_kws={"size": 8}, linewidths=0.4,
                cbar_kws={"shrink": 0.8})
    ax4.set_facecolor("#16213e")
    ax4.tick_params(colors="white", labelsize=8)
    ax4.set_title("Matriz de Correlação (Pearson)", color="white",
                  fontweight="bold", fontsize=10)

    # ── Painel 5: Outliers por feature ────────────────────────
    ax5 = fig.add_subplot(gs[1, 2])
    outlier_pcts = {}
    for feat in numeric_features:
        col = df[feat].dropna()
        Q1, Q3 = col.quantile(0.25), col.quantile(0.75)
        IQR = Q3 - Q1
        n_out = int(((col < Q1 - 1.5 * IQR) | (col > Q3 + 1.5 * IQR)).sum())
        outlier_pcts[feat] = round(n_out / len(col) * 100, 2)

    out_series = pd.Series(outlier_pcts).sort_values()
    colors_out = plt.cm.RdYlGn_r(np.linspace(0.1, 0.9, len(out_series)))
    bars5 = ax5.barh(out_series.index, out_series.values, color=colors_out)
    ax5.bar_label(bars5, fmt="%.1f%%", padding=3, color="white", fontsize=8)
    ax5.set_facecolor("#16213e")
    ax5.tick_params(colors="white", labelsize=8)
    for spine in ax5.spines.values(): spine.set_color("#444")
    ax5.set_title("% Outliers por Feature (IQR)", color="white",
                  fontweight="bold", fontsize=10)
    ax5.set_xlabel("% Outliers", color="white", fontsize=8)

    # ── Painel 6: Médias por classe ───────────────────────────
    ax6 = fig.add_subplot(gs[2, :])
    # Calcular média normalizada de cada feature por classe
    group_means = df.groupby(target_col)[numeric_features].mean()
    # Normalizar para [0,1] por feature
    group_means_norm = (group_means - group_means.min()) / (group_means.max() - group_means.min() + 1e-9)
    group_means_norm = group_means_norm.reindex(ordem_classes)

    x = np.arange(len(numeric_features))
    width = 0.15
    for j, (classe, row) in enumerate(group_means_norm.iterrows()):
        offset = (j - len(ordem_classes) / 2) * width
        color  = CLASS_PALETTE.get(classe, "#95a5a6")
        ax6.bar(x + offset, row.values, width, label=classe, color=color, alpha=0.85)

    ax6.set_xticks(x)
    ax6.set_xticklabels(numeric_features, rotation=20, color="white", fontsize=9)
    ax6.set_facecolor("#16213e")
    ax6.tick_params(colors="white")
    for spine in ax6.spines.values(): spine.set_color("#444")
    ax6.set_title("Média Normalizada por Feature e Classe", color="white",
                  fontweight="bold", fontsize=10)
    ax6.set_ylabel("Valor normalizado [0–1]", color="white", fontsize=9)
    ax6.legend(facecolor="#16213e", labelcolor="white", fontsize=8,
               loc="upper right", framealpha=0.7)

    path = _save_plot(fig, "eda_dashboard.png", output_dir)
    return {"eda_dashboard": path}



def run_eda(df, output_path, features, target_col, ordem_classes):
    """
    Executa a EDA completa e retorna dicionário com todos os caminhos de output.

    Parâmetros
    ----------
    df           : DataFrame bruto (sem split, sem encoding do target)
    output_path  : caminho do relatório HTML (ydata-profiling)
    features     : lista de colunas preditoras
    target_col   : nome da coluna alvo (deve conter strings, não inteiros)
    ordem_classes: lista com as classes na ordem desejada nos gráficos

    IMPORTANTE: o target_col deve ser string original (ex: "Moderada"),
    não a versão codificada pelo LabelEncoder. Encoding é responsabilidade
    do DataPreprocessor, não da EDA.
    """
    print("\n" + "=" * 60)
    print("    ANÁLISE EXPLORATÓRIA DE DADOS — INÍCIO")
    print("=" * 60)

    all_output_paths = {}
    reports_dir  = os.path.dirname(output_path) or "reports"
    figures_dir  = os.path.join(reports_dir, "figures")
    os.makedirs(figures_dir, exist_ok=True)

    # ── 1. Relatório ydata-profiling ───────────────────────────
    print("\n[1/8] Gerando relatório ydata-profiling...")
    profile = ProfileReport(
        df,
        title="Relatório de Qualidade Ambiental",
        explorative=True,
        minimal=False,
    )
    profile.to_file(output_path)
    all_output_paths["ydata_report"] = output_path
    print(f"  ✔ Relatório salvo: {output_path}")

    # ── 2. Distribuições ───────────────────────────────────────
    print("\n[2/8] Distribuições univariadas...")
    all_output_paths.update(plot_distributions(df, features, figures_dir))

    # ── 3. Balanceamento do target ─────────────────────────────
    print("\n[3/8] Balanceamento do target...")
    all_output_paths.update(plot_target_balance(df, target_col, ordem_classes, figures_dir))

    # ── 4. Correlação ──────────────────────────────────────────
    print("\n[4/8] Matriz de correlação...")
    all_output_paths.update(plot_correlation_matrix(df, features, figures_dir))

    # ── 5. Boxplots por classe ─────────────────────────────────
    print("\n[5/8] Boxplots por classe...")
    all_output_paths.update(
        plot_boxplots_by_class(df, features, target_col, ordem_classes, figures_dir)
    )

    # ── 6. Mapa de nulos ───────────────────────────────────────
    print("\n[6/8] Mapa de valores nulos...")
    all_output_paths.update(plot_null_map(df, figures_dir))

    # ── 7. Outliers IQR ────────────────────────────────────────
    print("\n[7/8] Detecção de outliers (IQR)...")
    detect_outliers_iqr(df, features)

    # ── 8. Pairplot ────────────────────────────────────────────
    print("\n[8/8] Pairplot (amostra)...")
    all_output_paths.update(
        plot_pairplot(df, features, target_col, ordem_classes, figures_dir)
    )

    # ── Dashboard consolidado ──────────────────────────────────
    print("\n[+] Dashboard consolidado...")
    all_output_paths.update(
        plot_eda_dashboard(df, features, target_col, ordem_classes, figures_dir)
    )

    print("\n" + "=" * 60)
    print("    EDA CONCLUÍDA")
    print(f"   {len(all_output_paths)} artefatos gerados em: {reports_dir}")
    print("=" * 60 + "\n")
    return all_output_paths



if __name__ == "__main__":
    import yaml

    with open("../config.yaml", "r") as f:
        config = yaml.safe_load(f)

    df = pd.read_csv("../data/dataset_ambiental.csv")

    features_cfg    = config["features"]["numerical"]
    target_col_cfg  = config["data"]["target"]

    
    ordem_classes_cfg = config["data"].get(
        "class_order",
        sorted(df[target_col_cfg].unique().tolist())
    )

    run_eda(
        df=df,
        output_path="../reports/eda_report.html",
        features=features_cfg,
        target_col=target_col_cfg,
        ordem_classes=ordem_classes_cfg,
    )