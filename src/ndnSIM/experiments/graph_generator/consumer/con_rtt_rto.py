import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_rtt_and_rto(input_directory, output_directory):
    try:
        # List all RTT and RTO files in the directory
        rtt_files = [f for f in os.listdir(input_directory) if f.startswith("con0_RTT")]
        rto_files = [f for f in os.listdir(input_directory) if f.startswith("con0_RTO")]

        # Dictionary to store RTT and RTO data by flow
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

        for rto_file in rto_files:
            flow = rto_file.split('_')[-1].split('.')[0]
            rto_path = os.path.join(input_directory, rto_file)
            rto_data = pd.read_csv(rto_path, sep="\s+", header=None, names=["Time", "RTO"], usecols=[0, 1])
            rto_data['Time'] /= 1_000  # Convert time to seconds
            #rto_data['RTO'] /= 1_000  # Convert RTO to milliseconds
            if flow not in flow_data:
                flow_data[flow] = {}
            flow_data[flow]['RTO'] = rto_data

        # Plot data for each flow
        for flow, data in flow_data.items():
            if 'RTT' in data and 'RTO' in data:
                plt.figure(figsize=(12, 6))
                ax1 = plt.gca()  # Primary axis for RTT
                ax2 = ax1.twinx()  # Secondary axis for RTO

                ax1.plot(data['RTT']['Time'], data['RTT']['RTT'], label=f'RTT {flow}', color='blue')
                ax2.plot(data['RTO']['Time'], data['RTO']['RTO'], label=f'RTO {flow}', color='green', linestyle='--')

                ax1.set_xlabel('Time (seconds)', fontsize=14)
                ax1.set_ylabel('RTT (ms)', fontsize=14, color='blue')
                ax2.set_ylabel('RTO (ms)', fontsize=14, color='green')
                plt.title(f"RTT vs RTO - Consumer's Flow {flow}", fontsize=16)

                ax1.legend(loc='upper left')
                ax2.legend(loc='upper right')

                plt.grid(True)
                plt.tight_layout()

                # Save the plot
                output_file = os.path.join(output_directory, f"con_rtt_rto_{flow}.png")
                plt.savefig(output_file, dpi=300)
                plt.close()
                print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"An error occurred: {e}")