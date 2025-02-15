import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_rtt_and_queue(input_directory, output_directory):
    try:
        # List all RTT and queue files in the directory
        rtt_files = [f for f in os.listdir(input_directory) if f.startswith("con0_RTT")]
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store RTT and Queue data by flow
        flow_data = {}

        for rtt_file in rtt_files:
            flow = rtt_file.split('_')[-1].split('.')[0]
            rtt_path = os.path.join(input_directory, rtt_file)
            rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
            rtt_data['Time'] /= 1_000  # Convert time to seconds
            #rtt_data['RTT'] /= 1_000  # Convert RTT to milliseconds
            if flow not in flow_data:
                flow_data[flow] = {}
            flow_data[flow]['RTT'] = rtt_data

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Queue"], usecols=[0, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            #queue_data['Queue'] /= 1_000  # Convert queue to milliseconds
            if flow not in flow_data:
                flow_data[flow] = {}
            flow_data[flow]['Queue'] = queue_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            if 'RTT' in data and 'Queue' in data:
                plt.figure(figsize=(12, 6))
                ax1 = plt.gca()  # Primary axis for RTT
                ax2 = ax1.twinx()  # Secondary axis for queue

                ax1.plot(data['RTT']['Time'], data['RTT']['RTT'], label=f'RTT {flow}', color='blue')
                ax2.plot(data['Queue']['Time'], data['Queue']['Queue'], label=f'Queue {flow}', color='green', linestyle='--')

                ax1.set_xlabel('Time (seconds)', fontsize=14)
                ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
                ax2.set_ylabel('Queue (packets)', fontsize=14, color='green')
                plt.title(f"RTT vs Queue - Consumer's Flow {flow}", fontsize=16)

                ax1.legend(loc='upper left')
                ax2.legend(loc='upper right')

                plt.grid(True)
                plt.tight_layout()

                # Save the plot
                output_file = os.path.join(output_directory, f"con_rtt_queue_{flow}.png")
                plt.savefig(output_file, dpi=300)
                plt.close()
                print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")




def plot_combined_rtt_and_queue(input_directory, output_directory):
    try:
        # List all RTT and queue files in the directory
        rtt_files = [f for f in os.listdir(input_directory) if f.startswith("con0_RTT")]
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store RTT and Queue data by flow
        flow_data = {}

        for rtt_file in rtt_files:
            flow = rtt_file.split('_')[-1].split('.')[0]
            rtt_path = os.path.join(input_directory, rtt_file)
            rtt_data = pd.read_csv(rtt_path, sep="\s+", header=None, names=["Time", "RTT"], usecols=[0, 2])
            rtt_data['Time'] /= 1_000  # Convert time to seconds
            if flow not in flow_data:
                flow_data[flow] = {}
            flow_data[flow]['RTT'] = rtt_data

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Queue"], usecols=[0, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            if flow not in flow_data:
                flow_data[flow] = {}
            flow_data[flow]['Queue'] = queue_data

        # Plot combined graph for all flows
        plt.figure(figsize=(12, 6))
        ax1 = plt.gca()  # Primary axis for RTT
        ax2 = ax1.twinx()  # Secondary axis for queue

        # Define separate color palettes for RTT and Queue
        rtt_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for RTT
        queue_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for Queue

        rtt_lines = []  # Stores RTT line handles for the legend
        queue_lines = []  # Stores Queue line handles for the legend

        for flow, data in flow_data.items():
            if 'RTT' in data and 'Queue' in data:
                # Plot RTT for each flow on the primary y-axis
                rtt_color = next(rtt_colors)
                line_rtt, = ax1.plot(
                    data['RTT']['Time'], data['RTT']['RTT'], label=f'RTT - Flow {flow}', color=rtt_color, linewidth=1.5
                )
                rtt_lines.append(line_rtt)

                # Plot Queue for each flow on the secondary y-axis
                queue_color = next(queue_colors)
                line_queue, = ax2.plot(
                    data['Queue']['Time'], data['Queue']['Queue'], label=f'Queue - Flow {flow}', color=queue_color, linestyle='--', linewidth=1.5
                )
                queue_lines.append(line_queue)

        # Set labels and title
        ax1.set_xlabel('Time (seconds)', fontsize=14)
        ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
        ax2.set_ylabel('Queue (packets)', fontsize=14, color='red')
        plt.title("Combined RTT vs Queue for Consumer Flows", fontsize=16)

        # Add legends for both metrics
        rtt_legend = ax1.legend(handles=rtt_lines, loc='upper left', fontsize=10, title="RTT")
        queue_legend = ax2.legend(handles=queue_lines, loc='upper right', fontsize=10, title="Queue")

        # Improve legend appearance
        rtt_legend.get_title().set_fontsize(12)
        queue_legend.get_title().set_fontsize(12)

        plt.grid(True)
        plt.tight_layout()

        # Save the combined plot
        output_file = os.path.join(output_directory, "con_rtt_queue.png")
        plt.savefig(output_file, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")        