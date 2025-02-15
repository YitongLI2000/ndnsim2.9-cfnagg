import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_rtt_and_queue(input_directory, output_directory):
    try:
        # List all files in the directory
        all_files = os.listdir(input_directory)
        rtt_files = [f for f in all_files if "RTT" in f]
        queue_files = [f for f in all_files if "queue" in f]

        # Dictionary to match rtt files with their corresponding queue files
        file_pairs = {}

        for rtt_file in rtt_files:
            # Extract identifiers to match the corresponding queue file
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]  # Extract the flow identifier
            # Create expected queue filename pattern for matching
            expected_queue_filename_patterns = [f"{agg_id}_queue_{flow}.txt"]

            # Find the corresponding queue file
            queue_file = next((cf for cf in queue_files if any(cf == expected for expected in expected_queue_filename_patterns)), None)
            if queue_file:
                file_pairs[rtt_file] = queue_file

        # Plot data for each pair of files
        for rtt_file, queue_file in file_pairs.items():
            # Update agg_id and flow for the current pair
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]

            rtt_path = os.path.join(input_directory, rtt_file)
            queue_path = os.path.join(input_directory, queue_file)

            rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Queue"], usecols=[0, 4])

            rtt_data['Time'] = rtt_data['Time'] / 1_000  # Convert to seconds
            queue_data['Time'] = queue_data['Time'] / 1_000  # Convert to seconds

            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for RTT
            ax2 = ax1.twinx()  # Secondary axis for queue

            ax1.plot(rtt_data['Time'], rtt_data['RTT'], label='RTT (ms)', color='blue')
            ax2.plot(queue_data['Time'], queue_data['Queue'], label='Queue (packets)', color='red', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
            ax2.set_ylabel('Queue (packets)', fontsize=14, color='red')
            plt.title(f'RTT vs Queue - {agg_id.upper()} Flow {flow}', fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on aggregator ID and flow
            output_file = os.path.join(output_directory, f"{agg_id}_rtt_queue_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")

def plot_combined_rtt_and_queue(input_directory, output_directory):
    try:
        # List all files in the directory
        all_files = os.listdir(input_directory)
        rtt_files = [f for f in all_files if "RTT" in f]
        queue_files = [f for f in all_files if "queue" in f]

        # Dictionary to match rtt files with their corresponding queue files
        file_pairs = {}

        for rtt_file in rtt_files:
            # Extract identifiers to match the corresponding queue file
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]  # Extract the flow identifier
            # Create expected queue filename pattern for matching
            expected_queue_filename_patterns = [f"{agg_id}_queue_{flow}.txt"]

            # Find the corresponding queue file
            queue_file = next((cf for cf in queue_files if any(cf == expected for expected in expected_queue_filename_patterns)), None)
            if queue_file:
                if agg_id not in file_pairs:
                    file_pairs[agg_id] = []
                file_pairs[agg_id].append((rtt_file, queue_file))

        # Plot data for each aggregator
        for agg_id, pairs in file_pairs.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for RTT
            ax2 = ax1.twinx()  # Secondary axis for queue

            # Define separate color palettes for RTT and queue
            rtt_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for RTT
            queue_colors = cycle(plt.cm.Reds(range(100, 255, 30)))  # Reds for queue

            rtt_lines = []  # For RTT legend handles
            queue_lines = []  # For queue legend handles

            # Plot data for each pair of files
            for rtt_file, queue_file in pairs:
                # Update flow for the current pair
                parts = rtt_file.split('_')
                flow = parts[2].split('.')[0]

                rtt_path = os.path.join(input_directory, rtt_file)
                queue_path = os.path.join(input_directory, queue_file)

                rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
                queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Queue"], usecols=[0, 4])

                rtt_data['Time'] = rtt_data['Time'] / 1_000  # Convert to seconds
                queue_data['Time'] = queue_data['Time'] / 1_000  # Convert to seconds

                # Plot RTT data on the primary y-axis
                rtt_color = next(rtt_colors)
                line_rtt, = ax1.plot(
                    rtt_data['Time'], rtt_data['RTT'], label=f'RTT (ms) Flow {flow}', color=rtt_color, linewidth=1.5
                )
                rtt_lines.append(line_rtt)

                # Plot queue data on the secondary y-axis
                queue_color = next(queue_colors)
                line_queue, = ax2.plot(
                    queue_data['Time'], queue_data['Queue'], label=f'Queue (packets) Flow {flow}', color=queue_color, linestyle='--', linewidth=1.5
                )
                queue_lines.append(line_queue)

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
            ax2.set_ylabel('Queue (packets)', fontsize=14, color='red')
            plt.title(f'Combined RTT vs Queue for {agg_id.upper()}', fontsize=16)

            # Add legends for both metrics
            rtt_legend = ax1.legend(handles=rtt_lines, loc='upper left', fontsize=10, title="RTT")
            queue_legend = ax2.legend(handles=queue_lines, loc='upper right', fontsize=10, title="Queue")

            # Improve legend appearance
            rtt_legend.get_title().set_fontsize(12)
            queue_legend.get_title().set_fontsize(12)

            plt.grid(True)
            plt.tight_layout()

            # Save the combined plot
            output_file = os.path.join(output_directory, f"{agg_id}_rtt_queue.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")