import pandas as pd
from itertools import combinations
from scipy.stats import ttest_rel, wilcoxon

# ------------------------------------------------------------
# Settings
# ------------------------------------------------------------
INPUT_CSV = "iterative_improvement_results.csv"
OUTPUT_CSV = "pairwise_algorithm_comparisons.csv"
ALPHA = 0.05

# ------------------------------------------------------------
# Load data
# ------------------------------------------------------------
df = pd.read_csv(INPUT_CSV)

# Expected columns:
# instance, pivoting_rule, neighborhood, initial_solution, cost, delta_percent, time_seconds

required_cols = {
    "instance",
    "pivoting_rule",
    "neighborhood",
    "initial_solution",
    "delta_percent"
}
missing = required_cols - set(df.columns)
if missing:
    raise ValueError(f"Missing required columns: {missing}")

# ------------------------------------------------------------
# Build algorithm label
# ------------------------------------------------------------
df["algorithm"] = (
    df["pivoting_rule"].astype(str) + "_" +
    df["neighborhood"].astype(str) + "_" +
    df["initial_solution"].astype(str)
)

# ------------------------------------------------------------
# Get all algorithms
# ------------------------------------------------------------
algorithms = sorted(df["algorithm"].unique())
print(f"Found {len(algorithms)} algorithms:")
for a in algorithms:
    print(" -", a)

# ------------------------------------------------------------
# Pairwise comparison function
# ------------------------------------------------------------
def compare_two_algorithms(df_all, alg1, alg2):
    df1 = df_all[df_all["algorithm"] == alg1][["instance", "delta_percent"]].copy()
    df2 = df_all[df_all["algorithm"] == alg2][["instance", "delta_percent"]].copy()

    df1 = df1.rename(columns={"delta_percent": "delta_1"})
    df2 = df2.rename(columns={"delta_percent": "delta_2"})

    merged = pd.merge(df1, df2, on="instance", how="inner").sort_values("instance")

    if merged.empty:
        raise ValueError(f"No common instances between {alg1} and {alg2}")

    x = merged["delta_1"].to_numpy()
    y = merged["delta_2"].to_numpy()

    # Mean values
    mean_1 = x.mean()
    mean_2 = y.mean()
    mean_diff = (x - y).mean()   # positive => alg1 worse if lower delta is better
    abs_mean_diff = abs(mean_diff)

    # Lower delta_percent is better
    if mean_1 < mean_2:
        better_mean = alg1
    elif mean_2 < mean_1:
        better_mean = alg2
    else:
        better_mean = "tie"

    # Paired t-test
    t_stat, p_ttest = ttest_rel(x, y, nan_policy="omit")

    # Wilcoxon signed-rank test
    # If all differences are exactly zero, wilcoxon fails
    diffs = x - y
    if (diffs == 0).all():
        w_stat = 0.0
        p_wilcoxon = 1.0
    else:
        try:
            w_stat, p_wilcoxon = wilcoxon(x, y, zero_method="wilcox")
        except ValueError:
            # fallback in edge cases
            w_stat, p_wilcoxon = float("nan"), float("nan")

    return {
        "algorithm_1": alg1,
        "algorithm_2": alg2,
        "n_instances": len(merged),
        "mean_delta_1": mean_1,
        "mean_delta_2": mean_2,
        "mean_diff_delta_1_minus_2": mean_diff,
        "abs_mean_diff": abs_mean_diff,
        "better_by_mean": better_mean,
        "t_statistic": t_stat,
        "p_value_ttest": p_ttest,
        "significant_ttest": p_ttest < ALPHA if pd.notna(p_ttest) else False,
        "wilcoxon_statistic": w_stat,
        "p_value_wilcoxon": p_wilcoxon,
        "significant_wilcoxon": p_wilcoxon < ALPHA if pd.notna(p_wilcoxon) else False,
    }

# ------------------------------------------------------------
# Compare all pairs
# ------------------------------------------------------------
results = []

for alg1, alg2 in combinations(algorithms, 2):
    res = compare_two_algorithms(df, alg1, alg2)
    results.append(res)

results_df = pd.DataFrame(results)

# Sort: strongest Wilcoxon significance first, then t-test
results_df = results_df.sort_values(
    by=["p_value_wilcoxon", "p_value_ttest", "abs_mean_diff"],
    ascending=[True, True, False]
).reset_index(drop=True)

# ------------------------------------------------------------
# Save
# ------------------------------------------------------------
results_df.to_csv(OUTPUT_CSV, index=False)

print(f"\nSaved pairwise comparison results to: {OUTPUT_CSV}")
print(f"Number of pairwise comparisons: {len(results_df)}")

# Optional preview
print("\nTop 10 comparisons:")
print(results_df.head(10).to_string(index=False, float_format=lambda x: f"{x:.6f}"))