import os
import pandas as pd
import matplotlib.pyplot as plt
from itertools import cycle

def plot_sendrate_and_inflight(input_directory, output_directory):
    try:
        # List all inflight files in the directory
        inflight_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store sendrate and inflight data by flow
        flow_data = {}

        for inflight_file in inflight_files:
            flow = inflight_file.split('_')[-1].split('.')[0]
            inflight_path = os.path.join(input_directory, inflight_file)
            inflight_data = pd.read_csv(inflight_path, sep="\s+", header=None, names=["Time", "SendRate", "InFlight"], usecols=[0, 1, 5])
            inflight_data['Time'] /= 1_000  # Convert time to seconds
            #inflight_data['SendRate'] /= 1_000  # Convert sendrate to appropriate units if needed
            #inflight_data['InFlight'] /= 1_000  # Convert InFlight to appropriate units if needed
            flow_data[flow] = inflight_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for SendRate
            ax2 = ax1.twinx()  # Secondary axis for inflight

            ax1.plot(data['Time'], data['SendRate'], label=f'SendRate {flow}', color='blue')
            ax2.plot(data['Time'], data['InFlight'], label=f'InFlight {flow}', color='green', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('SendRate (packets/ms)', fontsize=14, color='blue')
            ax2.set_ylabel('InFlight (packets)', fontsize=14, color='green')
            plt.title(f"SendRate vs InFlight - Consumer's Flow {flow}", fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot
            output_file = os.path.join(output_directory, f"con_sendrate_inflight_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")



def plot_combined_sendrate_and_inflight(input_directory, output_directory):
    try:
        # List all inflight files in the directory
        inflight_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store inflight and sendrate data by flow
        flow_data = {}

        for inflight_file in inflight_files:
            flow = inflight_file.split('_')[-1].split('.')[0]
            inflight_path = os.path.join(input_directory, inflight_file)
            inflight_data = pd.read_csv(inflight_path, sep="\s+", header=None, names=["Time", "SendRate", "InFlight"], usecols=[0, 1, 5])
            inflight_data['Time'] /= 1_000  # Convert time to seconds
            flow_data[flow] = inflight_data

        # Plot combined graph for all flows
        plt.figure(figsize=(12, 6))
        ax1 = plt.gca()  # Primary axis for inflight
        ax2 = ax1.twinx()  # Secondary axis for sendrate

        # Define separate color palettes for inflight and sendrate
        inflight_colors = cycle(plt.cm.Blues(range(100, 255, 30)))  # Blues for inflight
        sendrate_colors = cycle(plt.cm.Oranges(range(100, 255, 30)))  # Oranges for sendrate

        inflight_lines = []  # Stores inflight line handles for the legend
        sendrate_lines = []  # Stores sendrate line handles for the legend

        for flow, data in flow_data.items():
            # Plot inflight for each flow on the primary y-axis
            inflight_color = next(inflight_colors)
            line_inflight, = ax1.plot(
                data['Time'], data['InFlight'], label=f'InFlight - Flow {flow}', color=inflight_color, linewidth=1.5
            )
            inflight_lines.append(line_inflight)

            # Plot sendrate for each flow on the secondary y-axis
            sendrate_color = next(sendrate_colors)
            line_sendrate, = ax2.plot(data['Time'], data['SendRate'], label=f'SendRate - Flow {flow}', color=sendrate_color, linestyle='--', linewidth=1.5)
            sendrate_lines.append(line_sendrate)

        # Set labels and title
        ax1.set_xlabel('Time (seconds)', fontsize=14)
        ax1.set_ylabel('InFlight (packets)', fontsize=14, color='blue')
        ax2.set_ylabel('SendRate (packets/ms)', fontsize=14, color='red')
        plt.title("Combined InFlight vs SendRate for Consumer Flows", fontsize=16)

        # Add legends for both metrics
        inflight_legend = ax1.legend(handles=inflight_lines, loc='upper left', fontsize=10, title="InFlight")
        sendrate_legend = ax2.legend(handles=sendrate_lines, loc='upper right', fontsize=10, title="SendRate")

        # Improve legend appearance
        inflight_legend.get_title().set_fontsize(12)
        sendrate_legend.get_title().set_fontsize(12)

        plt.grid(True)
        plt.tight_layout()

        # Save the combined plot
        output_file = os.path.join(output_directory, "con_sendrate_inflight.png")
        plt.savefig(output_file, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")