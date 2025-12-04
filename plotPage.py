import matplotlib.pyplot as plt

# Reset to classic clean white style
plt.style.use("default")

FRAME_SIZES = [128, 256, 512, 1024]
ALGORITHMS = ["FIFO", "LRU", "OPT"]
CHECKPOINTS = [2000, 4000, 6000, 8000, 10000]


def parse_output_file(filename):
    total_faults = {algo: None for algo in ALGORITHMS}
    fault_rates = {algo: None for algo in ALGORITHMS}

    with open(filename, "r") as f:
        for line in f:
            stripped = line.strip()
            if not stripped:
                continue

            for algo in ALGORITHMS:
                if stripped.startswith(algo + " "):
                    parts = stripped.split()
                    total_faults[algo] = int(parts[1])
                    fault_rates[algo] = [float(x) for x in parts[2:2 + len(CHECKPOINTS)]]

    return total_faults, fault_rates


def main():
    total_faults = {algo: [] for algo in ALGORITHMS}
    fault_rates = {algo: [] for algo in ALGORITHMS}

    colors = {"FIFO": "tab:red", "LRU": "tab:green", "OPT": "tab:blue"}

    for frame in FRAME_SIZES:
        filename = f"output_{frame}.txt"
        print(f"Reading {filename} ...")
        tf, fr = parse_output_file(filename)

        for algo in ALGORITHMS:
            total_faults[algo].append(tf[algo])
            fault_rates[algo].append(fr[algo])

    # ----------------------------------------
    # Figure 1: Four-panel graph (the only one saved)
    # ----------------------------------------
    fig1, axes = plt.subplots(2, 2, figsize=(12, 9), facecolor="white")
    axes = axes.flatten()

    for idx, frame in enumerate(FRAME_SIZES):
        ax = axes[idx]
        ax.set_facecolor("white")

        for algo in ALGORITHMS:
            y = fault_rates[algo][idx]
            ax.plot(
                CHECKPOINTS, y,
                marker='o',
                linewidth=2,
                markersize=6,
                color=colors[algo],
                label=algo
            )

        ax.set_title(f"Frame Size = {frame}", fontsize=12, fontweight='bold')
        ax.set_xlabel("References", fontsize=10)
        ax.set_ylabel("Fault Rate", fontsize=10)
        ax.grid(True, linestyle="--", alpha=0.5)
        ax.legend()

    fig1.suptitle("Page Fault Rate vs Reference Count", fontsize=16, fontweight='bold')
    fig1.tight_layout(rect=[0, 0, 1, 0.95])

    # Save ONLY the 4-panel graph
    fig1.savefig("fault_rates_by_frame_size.png", dpi=300, bbox_inches="tight")

    # ----------------------------------------
    # Figure 2: Total faults (only shown, not saved)
    # ----------------------------------------
    plt.figure(figsize=(8, 6), facecolor="white")
    for algo in ALGORITHMS:
        plt.plot(
            FRAME_SIZES, total_faults[algo],
            marker='o', markersize=7,
            linewidth=2,
            color=colors[algo],
            label=algo
        )

    plt.xlabel("Frame Size", fontsize=12)
    plt.ylabel("Total Page Faults", fontsize=12)
    plt.title("Total Page Faults vs Frame Size", fontsize=14, fontweight="bold")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
