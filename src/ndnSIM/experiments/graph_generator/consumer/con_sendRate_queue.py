import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_sendrate_and_queue(input_directory, output_directory):
    try:
        # List all queue files in the directory
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store sendrate and queue data by flow
        flow_data = {}

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "SendRate", "Queue"], usecols=[0, 1, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            #queue_data['SendRate'] /= 1_000  # Convert sendrate to appropriate units if needed
            #queue_data['Queue'] /= 1_000  # Convert queue to appropriate units if needed
            flow_data[flow] = queue_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for SendRate
            ax2 = ax1.twinx()  # Secondary axis for queue

            ax1.plot(data['Time'], data['SendRate'], label=f'SendRate {flow}', color='blue')
            ax2.plot(data['Time'], data['Queue'], label=f'Queue {flow}', color='green', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('SendRate (packets/ms)', fontsize=14, color='blue')
            ax2.set_ylabel('Queue (packets)', fontsize=14, color='green')
            plt.title(f"SendRate vs Queue - Consumer's Flow {flow}", fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot
            output_file = os.path.join(output_directory, f"con_sendrate_queue_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")



def plot_combined_sendrate_and_queue(input_directory, output_directory):
    try:
        # List all queue files in the directory
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store queue and sendrate data by flow
        flow_data = {}

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "SendRate", "Queue"], usecols=[0, 1, 4])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            flow_data[flow] = queue_data

        # Plot combined graph for all flows
        plt.figure(figsize=(12, 6))
        ax1 = plt.gca()  # Primary axis for queue
        ax2 = ax1.twinx()  # Secondary axis for sendrate

        # Define separate color palettes for queue and sendrate
        queue_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for queue
        sendrate_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for sendrate

        queue_lines = []  # Stores queue line handles for the legend
        sendrate_lines = []  # Stores sendrate line handles for the legend

        for flow, data in flow_data.items():
            # Plot queue for each flow on the primary y-axis
            queue_color = next(queue_colors)
            line_queue, = ax1.plot(
                data['Time'], data['Queue'], label=f'Queue - Flow {flow}', color=queue_color, linewidth=1.5
            )
            queue_lines.append(line_queue)

            # Plot sendrate for each flow on the secondary y-axis
            sendrate_color = next(sendrate_colors)
            line_sendrate, = ax2.plot(data['Time'], data['SendRate'], label=f'SendRate - Flow {flow}', color=sendrate_color, linestyle='--', linewidth=1.5)
            sendrate_lines.append(line_sendrate)

        # Set labels and title
        ax1.set_xlabel('Time (seconds)', fontsize=14)
        ax1.set_ylabel('Queue (packets)', fontsize=14, color='blue')
        ax2.set_ylabel('SendRate (packets/ms)', fontsize=14, color='red')
        plt.title("Combined Queue vs SendRate for Consumer Flows", fontsize=16)

        # Add legends for both metrics
        queue_legend = ax1.legend(handles=queue_lines, loc='upper left', fontsize=10, title="Queue")
        sendrate_legend = ax2.legend(handles=sendrate_lines, loc='upper right', fontsize=10, title="SendRate")

        # Improve legend appearance
        queue_legend.get_title().set_fontsize(12)
        sendrate_legend.get_title().set_fontsize(12)

        plt.grid(True)
        plt.tight_layout()

        # Save the combined plot
        output_file = os.path.join(output_directory, "con_sendrate_queue.png")
        plt.savefig(output_file, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")