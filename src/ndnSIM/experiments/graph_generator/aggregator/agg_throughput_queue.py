import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_throughput_and_queue(input_directory, output_directory):
    try:
        # List all relevant files in the directory (assuming filenames contain "sendrate" or "throughput" indicators)
        all_files = [f for f in os.listdir(input_directory) if "queue" in f]

        # Process each file
        for file in all_files:
            # Extract identifiers for aggregator and flow from the filename
            parts = file.split('_')
            agg_id = parts[0]  # Aggregator ID
            flow = parts[2].split('.')[0]  # Flow identifier

            # Construct the full file path
            file_path = os.path.join(input_directory, file)

            # Read the file assuming it contains Time, Throughput, queue columns
            data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "Throughput", "Queue"], usecols=[0, 3, 4])

            data['Time'] = data['Time'] / 1_000  # Convert to seconds

            # Plotting
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for queue
            ax2 = ax1.twinx()  # Secondary axis for Throughput

            # Plot queue data on the primary y-axis
            ax1.plot(data['Time'], data['Queue'], label='Queue (packets)', color='blue')
            # Plot Throughput data on the secondary y-axis
            ax2.plot(data['Time'], data['Throughput'], label='Throughput (Mbps)', color='red', linestyle='--')

            # Set labels and title
            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('Queue (packets)', fontsize=14, color='blue')
            ax2.set_ylabel('Throughput (Mbps)', fontsize=14, color='red')
            plt.title(f'Queue vs Throughput - {agg_id.upper()} Flow {flow}', fontsize=16)

            # Add legends
            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on aggregator ID and flow
            output_file = os.path.join(output_directory, f"{agg_id}_throughput_queue_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")




def plot_combined_throughput_and_queue(input_directory, output_directory):
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

            # Read the file assuming it contains Time, Throughput, queue columns
            data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "Throughput", "Queue"], usecols=[0, 3, 4])

            data['Time'] = data['Time'] / 1_000  # Convert to seconds

            # Add flow data to the corresponding aggregator's group
            if agg_id not in aggregator_data:
                aggregator_data[agg_id] = {}
            aggregator_data[agg_id][f"flow_{flow}"] = data

        # Generate one plot per aggregator
        for agg_id, flows in aggregator_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for queue
            ax2 = ax1.twinx()  # Secondary axis for Throughput

            # Define separate color palettes for queue and Throughput
            queue_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for queue
            throughput_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for Throughput

            queue_lines = []  # For queue legend handles
            throughput_lines = []  # For Throughput legend handles

            # Plot all flows for the current aggregator
            for flow_name, data in flows.items():
                # Plot queue data on the primary y-axis
                queue_color = next(queue_colors)
                line_queue, = ax1.plot(
                    data['Time'], data['Queue'], label=f'Queue - {flow_name}', color=queue_color, linewidth=1.5
                )
                queue_lines.append(line_queue)

                # Plot Throughput data on the secondary y-axis
                throughput_color = next(throughput_colors)
                line_throughput, = ax2.plot(
                    data['Time'], data['Throughput'], label=f'Throughput - {flow_name}', color=throughput_color, linewidth=1.5
                )
                throughput_lines.append(line_throughput)

            # Set labels and title
            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('Queue (packets)', fontsize=14, color='blue')
            ax2.set_ylabel('Throughput (packets/ms)', fontsize=14, color='red')
            plt.title(f'Queue vs Throughput for {agg_id.upper()}', fontsize=16)

            # Add legends for both metrics
            queue_legend = ax1.legend(handles=queue_lines, loc='upper left', fontsize=10, title="Queue")
            throughput_legend = ax2.legend(handles=throughput_lines, loc='upper right', fontsize=10, title="Throughput")

            # Improve legend appearance
            queue_legend.get_title().set_fontsize(12)
            throughput_legend.get_title().set_fontsize(12)

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on the aggregator ID
            output_file = os.path.join(output_directory, f"{agg_id}_throughput_queue.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")