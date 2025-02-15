import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_sendRate_and_bw(input_directory, output_directory):
    try:
        # List all relevant files in the directory (assuming filenames contain "sendrate" or "bw" indicators)
        all_files = [f for f in os.listdir(input_directory) if "queue" in f]

        # Process each file
        for file in all_files:
            # Extract identifiers for aggregator and flow from the filename
            parts = file.split('_')
            agg_id = parts[0]  # Aggregator ID
            flow = parts[2].split('.')[0]  # Flow identifier

            # Construct the full file path
            file_path = os.path.join(input_directory, file)

            # Read the file assuming it contains Time, SendRate, BW columns
            data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "SendRate", "Bandwidth"], usecols=[0, 1, 2])

            data['Time'] = data['Time'] / 1_000  # Convert to seconds

            # Plotting
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for SendRate
            ax2 = ax1.twinx()  # Secondary axis for BW

            # Plot Sendrate data on the primary y-axis
            ax1.plot(data['Time'], data['SendRate'], label='SendRate (packets/ms)', color='blue')
            # Plot BW data on the secondary y-axis
            ax2.plot(data['Time'], data['Bandwidth'], label='Bandwidth (packets/ms)', color='red', linestyle='--')

            # Set labels and title
            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('SendRate (packets/ms)', fontsize=14, color='blue')
            ax2.set_ylabel('Bandwidth (packets/ms)', fontsize=14, color='red')
            plt.title(f'SendRate vs Bandwidth - {agg_id.upper()} Flow {flow}', fontsize=16)

            # Add legends
            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on aggregator ID and flow
            output_file = os.path.join(output_directory, f"{agg_id}_sendRate_BW_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")

