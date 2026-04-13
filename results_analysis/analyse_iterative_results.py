import pandas as pd
import os 

# ------------------------------------------------------------
# Load data
# ------------------------------------------------------------
csv_file = "iterative_improvement_results.csv"
df = pd.read_csv(csv_file)

# Expected columns:
# instance, pivoting_rule, neighborhood, initial_solution, cost, delta_percent, time_seconds

# ------------------------------------------------------------
# Basic checks
# ------------------------------------------------------------
print("Shape:", df.shape)
print("Columns:", list(df.columns))
print("\nUnique pivoting rules:", sorted(df["pivoting_rule"].unique()))
print("Unique neighborhoods:", sorted(df["neighborhood"].unique()))
print("Unique initial solutions:", sorted(df["initial_solution"].unique()))
print("Number of instances:", df["instance"].nunique())

# ------------------------------------------------------------
# Table 1: Main results for the 12 iterative improvement algorithms
# ------------------------------------------------------------
table1 = (
    df.groupby(["pivoting_rule", "neighborhood", "initial_solution"], as_index=False)
      .agg(
          avg_deviation_percent=("delta_percent", "mean"),
          std_deviation_percent=("delta_percent", "std"),
          total_time_seconds=("time_seconds", "sum"),
          avg_time_per_instance_seconds=("time_seconds", "mean")
      )
)

table1 = table1.sort_values(
    by=["avg_deviation_percent", "total_time_seconds"],
    ascending=[True, True]
).reset_index(drop=True)

print("\n=== TABLE 1: Main results for the 12 algorithms ===")
print(table1.to_string(index=False, float_format=lambda x: f"{x:.6f}"))

# ------------------------------------------------------------
# Table 2: Best algorithms by quality and by speed
# ------------------------------------------------------------
best_quality = table1.loc[table1["avg_deviation_percent"].idxmin()]
second_best_quality = table1.nsmallest(2, "avg_deviation_percent").iloc[1]
fastest = table1.loc[table1["total_time_seconds"].idxmin()]
slowest = table1.loc[table1["total_time_seconds"].idxmax()]

cw_only = table1[table1["initial_solution"] == "cw"]
fastest_cw = cw_only.loc[cw_only["total_time_seconds"].idxmin()]

table2 = pd.DataFrame([
    {
        "criterion": "Best average deviation",
        "pivoting_rule": best_quality["pivoting_rule"],
        "neighborhood": best_quality["neighborhood"],
        "initial_solution": best_quality["initial_solution"],
        "value": best_quality["avg_deviation_percent"]
    },
    {
        "criterion": "Second best average deviation",
        "pivoting_rule": second_best_quality["pivoting_rule"],
        "neighborhood": second_best_quality["neighborhood"],
        "initial_solution": second_best_quality["initial_solution"],
        "value": second_best_quality["avg_deviation_percent"]
    },
    {
        "criterion": "Fastest total time",
        "pivoting_rule": fastest["pivoting_rule"],
        "neighborhood": fastest["neighborhood"],
        "initial_solution": fastest["initial_solution"],
        "value": fastest["total_time_seconds"]
    },
    {
        "criterion": "Fastest CW-based algorithm",
        "pivoting_rule": fastest_cw["pivoting_rule"],
        "neighborhood": fastest_cw["neighborhood"],
        "initial_solution": fastest_cw["initial_solution"],
        "value": fastest_cw["total_time_seconds"]
    },
    {
        "criterion": "Slowest algorithm",
        "pivoting_rule": slowest["pivoting_rule"],
        "neighborhood": slowest["neighborhood"],
        "initial_solution": slowest["initial_solution"],
        "value": slowest["total_time_seconds"]
    }
])

print("\n=== TABLE 2: Best/worst algorithms ===")
print(table2.to_string(index=False, float_format=lambda x: f"{x:.6f}"))

# ------------------------------------------------------------
# Table 3: Effect of the initial solution
# ------------------------------------------------------------
table3 = (
    table1.groupby("initial_solution", as_index=False)
          .agg(mean_of_avg_deviations=("avg_deviation_percent", "mean"))
)

print("\n=== TABLE 3: Effect of the initial solution ===")
print(table3.to_string(index=False, float_format=lambda x: f"{x:.6f}"))

if set(table3["initial_solution"]) == {"cw", "random"}:
    cw_mean = table3.loc[table3["initial_solution"] == "cw", "mean_of_avg_deviations"].iloc[0]
    rand_mean = table3.loc[table3["initial_solution"] == "random", "mean_of_avg_deviations"].iloc[0]
    print(f"\nCW reduces the mean deviation by {rand_mean - cw_mean:.6f} percentage points.")

# ------------------------------------------------------------
# Table 4: Effect of pivoting rule within same initialization and neighborhood
# ------------------------------------------------------------
table4 = (
    table1.pivot_table(
        index=["initial_solution", "neighborhood"],
        columns="pivoting_rule",
        values="avg_deviation_percent"
    )
    .reset_index()
)

if "first" in table4.columns and "best" in table4.columns:
    table4["better_one"] = table4.apply(
        lambda row: "best" if row["best"] < row["first"] else ("first" if row["first"] < row["best"] else "tie"),
        axis=1
    )

print("\n=== TABLE 4: First vs Best within each neighborhood and initialization ===")
print(table4.to_string(index=False, float_format=lambda x: f"{x:.6f}"))

# ------------------------------------------------------------
# Table 5: Effect of neighborhood within same pivoting rule and initialization
# ------------------------------------------------------------
table5 = (
    table1.pivot_table(
        index=["pivoting_rule", "initial_solution"],
        columns="neighborhood",
        values="avg_deviation_percent"
    )
    .reset_index()
)

def best_neighborhood(row):
    candidates = {}
    for n in ["transpose", "exchange", "insert"]:
        if n in row.index and pd.notna(row[n]):
            candidates[n] = row[n]
    if not candidates:
        return None
    best_val = min(candidates.values())
    best_names = [k for k, v in candidates.items() if v == best_val]
    return " / ".join(best_names)

table5["best_neighborhood"] = table5.apply(best_neighborhood, axis=1)

print("\n=== TABLE 5: Neighborhood comparison within pivoting rule and initialization ===")
print(table5.to_string(index=False, float_format=lambda x: f"{x:.6f}"))

# ------------------------------------------------------------
# Optional: save tables to CSV files
# ------------------------------------------------------------

output_dir = "analyse"

table1.to_csv(os.path.join(output_dir, "table1_main_results.csv"), index=False)
table2.to_csv(os.path.join(output_dir, "table2_best_worst.csv"), index=False)
table3.to_csv(os.path.join(output_dir, "table3_initial_solution_effect.csv"), index=False)
table4.to_csv(os.path.join(output_dir, "table4_first_vs_best.csv"), index=False)
table5.to_csv(os.path.join(output_dir, "table5_neighborhood_effect.csv"), index=False)

print("\nSaved:")
print(" - table1_main_results.csv")
print(" - table2_best_worst.csv")
print(" - table3_initial_solution_effect.csv")
print(" - table4_first_vs_best.csv")
print(" - table5_neighborhood_effect.csv")