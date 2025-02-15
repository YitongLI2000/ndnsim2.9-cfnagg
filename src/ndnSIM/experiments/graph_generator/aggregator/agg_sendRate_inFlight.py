import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_sendRate_and_inflight(input_directory, output_directory):
    try:
        # List all relevant files in the directory (assuming filenames contain "sendrate" or "inflight" indicators)
        all_files = [f for f in os.listdir(input_directory) if "queue" in f]

        # Process each file
        for file in all_files:
            # Extract identifiers for aggregator and flow from the filename
            parts = file.split('_')
            agg_id = parts[0]  # Aggregator ID
            flow = parts[2].split('.')[0]  # Flow identifier

            # Construct the full file path
            file_path = os.path.join(input_directory, file)

            # Read the file assuming it contains Time, SendRate, inflight columns
            data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "SendRate", "InFlight"], usecols=[0, 1, 5])

            data['Time'] = data['Time'] / 1_000  # Convert to seconds

            # Plotting
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for SendRate
            ax2 = ax1.twinx()  # Secondary axis for inflight

            # Plot Sendrate data on the primary y-axis
            ax1.plot(data['Time'], data['SendRate'], label='SendRate (packets/ms)', color='blue')
            # Plot inflight data on the secondary y-axis
            ax2.plot(data['Time'], data['InFlight'], label='InFlight (packets)', color='red', linestyle='--')

            # Set labels and title
            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('SendRate (packets/ms)', fontsize=14, color='blue')
            ax2.set_ylabel('InFlight (packets)', fontsize=14, color='red')
            plt.title(f'SendRate vs InFlight - {agg_id.upper()} Flow {flow}', fontsize=16)

            # Add legends
            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on aggregator ID and flow
            output_file = os.path.join(output_directory, f"{agg_id}_sendRate_inflight_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")




def plot_combined_sendRate_and_inflight(input_directory, output_directory):
    try:
        # List all relevant files in the directory (assuming filenames contain "queue" indicators)
        all_files = [f for f in os.listdir(input_directory) if "queue" in f]

        # Dictionary to group files by aggregator
        aggregator_data = {}

        # Group files by aggregator
        for file in all_files:
            # Extract identifiers for aggregator and flow from the filename
            parts = file.split('_')
            agg_id = parts[0]  # Aggregator ID
            flow = parts[2].split('.')[0]  # Flow identifier

            # Construct the full file path
            file_path = os.path.join(input_directory, file)

            # Read the file assuming it contains Time, sendrate, inflight columns
            data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "SendRate", "InFlight"], usecols=[0, 1, 5])

            data['Time'] = data['Time'] / 1_000  # Convert to seconds

            # Add flow data to the corresponding aggregator's group
            if agg_id not in aggregator_data:
                aggregator_data[agg_id] = {}
            aggregator_data[agg_id][f"flow_{flow}"] = data

        # Generate one plot per aggregator
        for agg_id, flows in aggregator_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for inflight
            ax2 = ax1.twinx()  # Secondary axis for sendrate

            # Define separate color palettes for inflight and sendrate
            inflight_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for inflight
            sendrate_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for sendrate

            inflight_lines = []  # For inflight legend handles
            sendrate_lines = []  # For sendrate legend handles

            # Plot all flows for the current aggregator
            for flow_name, data in flows.items():
                # Plot inflight data on the primary y-axis
                inflight_color = next(inflight_colors)
                line_inflight, = ax1.plot(
                    data['Time'], data['InFlight'], label=f'InFlight - {flow_name}', color=inflight_color, linewidth=1.5
                )
                inflight_lines.append(line_inflight)

                # Plot sendrate data on the secondary y-axis
                sendrate_color = next(sendrate_colors)
                line_sendrate, = ax2.plot(
                    data['Time'], data['SendRate'], label=f'SendRate - {flow_name}', color=sendrate_color, linewidth=1.5
                )
                sendrate_lines.append(line_sendrate)

            # Set labels and title
            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('InFlight (packets)', fontsize=14, color='blue')
            ax2.set_ylabel('SendRate (packets/ms)', fontsize=14, color='red')
            plt.title(f'InFlight vs SendRate for {agg_id.upper()}', fontsize=16)

            # Add legends for both metrics
            inflight_legend = ax1.legend(handles=inflight_lines, loc='upper left', fontsize=10, title="InFlight")
            sendrate_legend = ax2.legend(handles=sendrate_lines, loc='upper right', fontsize=10, title="SendRate")

            # Improve legend appearance
            inflight_legend.get_title().set_fontsize(12)
            sendrate_legend.get_title().set_fontsize(12)

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on the aggregator ID
            output_file = os.path.join(output_directory, f"{agg_id}_sendrate_inflight.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")