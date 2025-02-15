import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_throughput_and_queue(input_directory, output_directory):
    try:
        # List all queue files in the directory
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store queue and throughput data by flow
        flow_data = {}

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Throughput", "Queue"], usecols=[0, 3, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            flow_data[flow] = queue_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for queue
            ax2 = ax1.twinx()  # Secondary axis for throughput

            ax1.plot(data['Time'], data['Throughput'], label=f'Throughput {flow}', color='blue')
            ax2.plot(data['Time'], data['Queue'], label=f'Queue {flow}', color='green', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('Throughput (Mbps)', fontsize=14, color='blue')
            ax2.set_ylabel('Queue (packets)', fontsize=14, color='green')
            plt.title(f"Throughput vs Queue - Consumer's Flow {flow}", fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot
            output_file = os.path.join(output_directory, f"con_throughput_queue_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")


def plot_combined_throughput_and_queue(input_directory, output_directory):
    try:
        # List all queue files in the directory
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store queue and throughput data by flow
        flow_data = {}

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "Throughput", "Queue"], usecols=[0, 3, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            flow_data[flow] = queue_data

        # Plot combined graph for all flows
        plt.figure(figsize=(12, 6))
        ax1 = plt.gca()  # Primary axis for queue
        ax2 = ax1.twinx()  # Secondary axis for throughput

        # Define separate color palettes for queue and throughput
        queue_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for queue
        throughput_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for throughput

        queue_lines = []  # Stores queue line handles for the legend
        throughput_lines = []  # Stores throughput line handles for the legend

        for flow, data in flow_data.items():
            # Plot queue for each flow on the primary y-axis
            queue_color = next(queue_colors)
            line_queue, = ax1.plot(
                data['Time'], data['Queue'], label=f'Queue - Flow {flow}', color=queue_color, linewidth=1.5
            )
            queue_lines.append(line_queue)

            # Plot throughput for each flow on the secondary y-axis
            throughput_color = next(throughput_colors)
            line_throughput, = ax2.plot(data['Time'], data['Throughput'], label=f'Throughput - Flow {flow}', color=throughput_color, linestyle='--', linewidth=1.5)
            throughput_lines.append(line_throughput)

        # Set labels and title
        ax1.set_xlabel('Time (seconds)', fontsize=14)
        ax1.set_ylabel('Queue (packets)', fontsize=14, color='blue')
        ax2.set_ylabel('Throughput (Mbps)', fontsize=14, color='red')
        plt.title("Combined Queue vs Throughput for Consumer Flows", fontsize=16)

        # Add legends for both metrics
        queue_legend = ax1.legend(handles=queue_lines, loc='upper left', fontsize=10, title="Queue")
        throughput_legend = ax2.legend(handles=throughput_lines, loc='upper right', fontsize=10, title="Throughput")

        # Improve legend appearance
        queue_legend.get_title().set_fontsize(12)
        throughput_legend.get_title().set_fontsize(12)

        plt.grid(True)
        plt.tight_layout()

        # Save the combined plot
        output_file = os.path.join(output_directory, "con_throughput_queue.png")
        plt.savefig(output_file, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")