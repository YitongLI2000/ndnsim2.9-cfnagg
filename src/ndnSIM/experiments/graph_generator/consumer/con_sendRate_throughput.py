import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_sendrate_and_throughput(input_directory, output_directory):
    try:
        # List all queue files in the directory
        queue_files = [f for f in os.listdir(input_directory) if f.startswith("con0_queue")]

        # Dictionary to store sendrate and throughput data by flow
        flow_data = {}

        for queue_file in queue_files:
            flow = queue_file.split('_')[-1].split('.')[0]
            queue_path = os.path.join(input_directory, queue_file)
            queue_data = pd.read_csv(queue_path, sep="\s+", header=None, names=["Time", "SendRate", "Throughput"], usecols=[0, 1, 3])
            queue_data['Time'] /= 1_000  # Convert time to seconds
            #queue_data['SendRate'] /= 1_000  # Convert sendrate to appropriate units if needed
            #queue_data['Throughput'] /= 1_000  # Convert bandwidth to appropriate units if needed
            flow_data[flow] = queue_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            plt.figure(figsize=(12, 6))
            ax1 = plt.gca()  # Primary axis for SendRate
            ax2 = ax1.twinx()  # Secondary axis for throughput

            ax1.plot(data['Time'], data['SendRate'], label=f'SendRate {flow}', color='blue')
            ax2.plot(data['Time'], data['Throughput'], label=f'Throughput {flow}', color='green', linestyle='--')

            ax1.set_xlabel('Time (seconds)', fontsize=14)
            ax1.set_ylabel('SendRate (packets/ms)', fontsize=14, color='blue')
            ax2.set_ylabel('Throughput (Mbps)', fontsize=14, color='green')
            plt.title(f"SendRate vs Throughput - Consumer's Flow {flow}", fontsize=16)

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            plt.grid(True)
            plt.tight_layout()

            # Save the plot
            output_file = os.path.join(output_directory, f"con_sendrate_throughput_{flow}.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")
