import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_throughput(input_directory, output_directory):
    try:
        file_prefix = "fwd"
        forwarder_data = {}

        # Look for log files named like fwd_<nodeName>.txt
        for file in os.listdir(input_directory):
            if file.startswith(file_prefix + "_") and file.endswith(".txt"):
                parts = file.split('_')  # e.g., ["fwd", "NodeA.txt"]
                forwarder_name = parts[1].split('.')[0]  # "NodeA"
                file_path = os.path.join(input_directory, file)

                # Read columns [0, 3] for Time and Throughput
                data = pd.read_csv(file_path, sep="\s+", header=None,
                                   names=["Time", "Throughput"],
                                   usecols=[0, 1])
                data["Time"] = data["Time"] / 1000.0  # Convert to seconds

                # Change throughput from pkgs/ms into Mbps, currently using 150 double
                #data["Throughput"] = data["Throughput"] * 1000 * 8 * 8 * 150 / 1_000_000  # Convert to Mbps

                forwarder_data[forwarder_name] = data

        # Plot throughput for each forwarder
        for forwarder_name, df in forwarder_data.items():
            plt.figure(figsize=(12, 6))
            plt.plot(df["Time"], df["Throughput"], label=f'Forwarder {forwarder_name}')
            plt.xlabel("Time (seconds)", fontsize=14)
            plt.ylabel("Throughput (Mbps)", fontsize=14)
            plt.title(f"Throughput - Forwarder {forwarder_name}", fontsize=16)
            plt.legend(loc="upper right", fontsize=8)
            plt.grid(True)
            plt.tight_layout()

            # Save plot
            output_file = os.path.join(output_directory, f"fwd_{forwarder_name}_throughput.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"Error occurred: {e}")