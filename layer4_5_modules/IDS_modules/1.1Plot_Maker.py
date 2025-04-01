import matplotlib.pyplot as plt
import pandas as pd

CSV_FILENAME = "1.1dns_query_log.csv"


def plot_dns_queries():
    # Load CSV data
    df = pd.read_csv(CSV_FILENAME)

    # Check if data exists
    if df.empty:
        print("No data to plot.")
        return

    # Convert elapsed time to integer
    df["Elapsed Time (s)"] = df["Elapsed Time (s)"].astype(int)

    # Filter out the first 5 seconds of data
    df = df[df["Elapsed Time (s)"] > 5]

    # Adjust elapsed time to start at 0 after the first 5 seconds
    df["Elapsed Time (s)"] = df["Elapsed Time (s)"] - 5

    # Pivot data so each IP gets its own line
    pivot_df = df.pivot(
        index="Elapsed Time (s)", columns="IP Address", values="Queries"
    ).fillna(0)

    # Create a mapping of IP addresses to unique labels
    ip_to_label = {
        "192.168.0.13": "Attacker 1",
        "192.168.0.12": "Attacker 2",
        "192.168.0.19": "Attacker 3",
        # Add more mappings as needed
    }

    # Plot each IP's query rate
    ax = pivot_df.plot(figsize=(10, 6), marker="o")

    # Formatting
    ax.set_xlabel("Elapsed Time (s)", fontsize=18)
    ax.set_ylabel("DNS Query Rate", fontsize=18)
    # ax.set_title(INSERT TITLE HERE)
    ax.grid(True)

    # Set y-axis to logarithmic scale
    ax.set_yscale("log")
    ax.tick_params(axis="y", which="major", labelsize=16)

    # Limit x-axis to 55 seconds (0 to 55 after filtering out the first 5 seconds)
    ax.set_xlim(0, 55)

    # Adjust x-axis labels to start at 0 and go to 25
    x_ticks = range(0, 55, 5)  # Adjusted x-axis values
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_ticks, fontsize=16)

    # Move legend inside the plot and re-label IP addresses
    handles, labels = ax.get_legend_handles_labels()
    new_labels = [
        ip_to_label.get(label, label) for label in labels
    ]  # Re-label IP addresses

    sorted_handles_labels = sorted(zip(new_labels, handles), key=lambda x: x[0])
    sorted_labels, sorted_handles = zip(*sorted_handles_labels)

    ax.legend(sorted_handles, sorted_labels, loc="center", frameon=True, fontsize=16)

    # Show plot
    plt.tight_layout()
    plt.show()


# Call function to plot DNS query rate
plot_dns_queries()
