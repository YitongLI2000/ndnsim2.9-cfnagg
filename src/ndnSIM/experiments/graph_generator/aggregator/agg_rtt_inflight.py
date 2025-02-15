import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_rtt_and_inflight(input_directory, output_directory):
    try:
        # List all files in the directory
        all_files = os.listdir(input_directory)
        rtt_files = [f for f in all_files if "RTT" in f]
        inflight_files = [f for f in all_files if "queue" in f]

        # Dictionary to match rtt files with their corresponding inflight files
        file_pairs = {}

        for rtt_file in rtt_files:
            # Extract identifiers to match the corresponding queinflightue file
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]  # Extract the flow identifier
            # Create expected inflight filename pattern for matching
            expected_inflight_filename_patterns = [f"{agg_id}_queue_{flow}.txt"]

            # Find the corresponding inflight file
            inflight_file = next((cf for cf in inflight_files if any(cf == expected for expected in expected_inflight_filename_patterns)), None)
            if inflight_file:
                file_pairs[rtt_file] = inflight_file

        # Plot data for each pair of files
        for rtt_file, inflight_file in file_pairs.items():
            # Update agg_id and flow for the current pair
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]

            rtt_path = os.path.join(input_directory, rtt_file)
            inflight_path = os.path.join(input_directory, inflight_file)

            rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
            inflight_data = pd.read_csv(inflight_path, sep="\s+", header=None, names=["Time", "InFlight"], usecols=[0, 5])

            rtt_data['Time'] = rtt_data['Time'] / 1_000  # Convert to seconds
            inflight_data['Time'] = inflight_data['Time'] / 1_000  # Convert to seconds

            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for RTT
            ax2 = ax1.twinx()  # Secondary axis for inflight

            ax1.plot(rtt_data['Time'], rtt_data['RTT'], label='RTT (ms)', color='blue')
            ax2.plot(inflight_data['Time'], inflight_data['InFlight'], label='InFlight (packets)', color='red', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
            ax2.set_ylabel('InFlight (packets)', fontsize=14, color='red')
            plt.title(f'RTT vs InFlight - {agg_id.upper()} Flow {flow}', fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot with a unique name based on aggregator ID and flow
            output_file = os.path.join(output_directory, f"{agg_id}_rtt_inflight_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")

def plot_combined_rtt_and_inflight(input_directory, output_directory):
    try:
        # List all files in the directory
        all_files = os.listdir(input_directory)
        rtt_files = [f for f in all_files if "RTT" in f]
        inflight_files = [f for f in all_files if "queue" in f]

        # Dictionary to match rtt files with their corresponding inflight files
        file_pairs = {}

        for rtt_file in rtt_files:
            # Extract identifiers to match the corresponding inflight file
            parts = rtt_file.split('_')
            agg_id = parts[0]
            flow = parts[2].split('.')[0]  # Extract the flow identifier
            # Create expected inflight filename pattern for matching
            expected_inflight_filename_patterns = [f"{agg_id}_queue_{flow}.txt"]

            # Find the corresponding inflight file
            inflight_file = next((cf for cf in inflight_files if any(cf == expected for expected in expected_inflight_filename_patterns)), None)
            if inflight_file:
                if agg_id not in file_pairs:
                    file_pairs[agg_id] = []
                file_pairs[agg_id].append((rtt_file, inflight_file))

        # Plot data for each aggregator
        for agg_id, pairs in file_pairs.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for RTT
            ax2 = ax1.twinx()  # Secondary axis for inflight

            # Define separate color palettes for RTT and inflight
            rtt_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for RTT
            inflight_colors = cycle(plt.cm.Reds(range(100, 255, 30)))  # Reds for inflight

            rtt_lines = []  # For RTT legend handles
            inflight_lines = []  # For inflight legend handles

            # Plot data for each pair of files
            for rtt_file, inflight_file in pairs:
                # Update flow for the current pair
                parts = rtt_file.split('_')
                flow = parts[2].split('.')[0]

                rtt_path = os.path.join(input_directory, rtt_file)
                inflight_path = os.path.join(input_directory, inflight_file)

                rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
                inflight_data = pd.read_csv(inflight_path, sep="\s+", header=None, names=["Time", "InFlight"], usecols=[0, 5])

                rtt_data['Time'] = rtt_data['Time'] / 1_000  # Convert to seconds
                inflight_data['Time'] = inflight_data['Time'] / 1_000  # Convert to seconds

                # Plot RTT data on the primary y-axis
                rtt_color = next(rtt_colors)
                line_rtt, = ax1.plot(
                    rtt_data['Time'], rtt_data['RTT'], label=f'RTT (ms) Flow {flow}', color=rtt_color, linewidth=1.5
                )
                rtt_lines.append(line_rtt)

                # Plot inflight data on the secondary y-axis
                inflight_color = next(inflight_colors)
                line_inflight, = ax2.plot(
                    inflight_data['Time'], inflight_data['InFlight'], label=f'InFlight (packets) Flow {flow}', color=inflight_color, linestyle='--', linewidth=1.5
                )
                inflight_lines.append(line_inflight)

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
            ax2.set_ylabel('InFlight (packets)', fontsize=14, color='red')
            plt.title(f'Combined RTT vs InFlight for {agg_id.upper()}', fontsize=16)

            # Add legends for both metrics
            rtt_legend = ax1.legend(handles=rtt_lines, loc='upper left', fontsize=10, title="RTT")
            inflight_legend = ax2.legend(handles=inflight_lines, loc='upper right', fontsize=10, title="InFlight")

            # Improve legend appearance
            rtt_legend.get_title().set_fontsize(12)
            inflight_legend.get_title().set_fontsize(12)

            plt.grid(True)
            plt.tight_layout()

            # Save the combined plot
            output_file = os.path.join(output_directory, f"{agg_id}_rtt_inflight.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")