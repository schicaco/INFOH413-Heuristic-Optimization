import pandas as pd

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------
INPUT_RESULTS = "iterative_improvement_results.csv"
BEST_KNOWN_FILE = "../best_known/best_known.txt"
OUTPUT_FILE = "iterative_improvement_results_with_best.csv"

# ------------------------------------------------------------
# Load results CSV
# ------------------------------------------------------------
df = pd.read_csv(INPUT_RESULTS)

# Check instance column
if "instance" not in df.columns:
    raise ValueError("Column 'instance' not found in CSV")

# ------------------------------------------------------------
# Load best_known.txt
# ------------------------------------------------------------
best_known_dict = {}

with open(BEST_KNOWN_FILE, "r") as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) >= 2:
            instance_name = parts[0].strip()
            value = int(parts[1])
            best_known_dict[instance_name] = value

# Debug print
print(f"Loaded {len(best_known_dict)} best-known values")

# ------------------------------------------------------------
# Match instance names
# ------------------------------------------------------------
def extract_instance_name(full_path):
    """
    If your CSV has full paths like:
    ./instances/N-be75eec_150.dat
    → extract N-be75eec_150
    """
    name = full_path.split("/")[-1]      # remove path
    name = name.replace(".dat", "")      # remove extension
    return name

# Apply extraction
df["instance_clean"] = df["instance"].apply(extract_instance_name)

# Map best_known values
df["best_known"] = df["instance_clean"].map(best_known_dict)

# ------------------------------------------------------------
# Check for missing matches
# ------------------------------------------------------------
missing = df[df["best_known"].isna()]["instance_clean"].unique()

if len(missing) > 0:
    print("\n WARNING: Missing best-known values for:")
    for m in missing:
        print(" -", m)
else:
    print("\nAll instances matched successfully!")


df = df[["instance", "pivoting_rule", "neighborhood", "initial_solution","best_known" ,"cost", "delta_percent", "time_seconds"]]

# ------------------------------------------------------------
# Save new CSV
# ------------------------------------------------------------
df.to_csv(OUTPUT_FILE, index=False)

print(f"\nSaved file with best_known column → {OUTPUT_FILE}")