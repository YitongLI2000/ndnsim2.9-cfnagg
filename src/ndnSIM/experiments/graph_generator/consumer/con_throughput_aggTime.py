import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_throughput_and_agg_time(input_directory, output_directory):
    try:
        # Initialize dictionaries to store data from each flow
        throughput_data = {}

        # Iterate over files in the throughput directory
        files = os.listdir(input_directory)
        for file in files:
            if file.startswith("con0_queue"):
                parts = file.split('_')
                flow = parts[-1].split('.')[0]  # Extract the flow identifier, assuming file format is correctly followed
                file_path = os.path.join(input_directory, file)

                # Read throughput data from the file
                data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "Throughput"], usecols=[0, 3])
                data['Time'] = data['Time'] / 1_000  # Convert time to seconds for uniformity
                #data['Throughput'] = data['Throughput'] / 1_000  # Convert Throughput to milliseconds
                throughput_data[flow] = data

        agg_time_file = os.path.join(input_directory, "con0_aggregationTime.txt")

        # Read the single aggregation time data file
        agg_data = pd.read_csv(agg_time_file, sep="\s+", header=None, names=["Time", "AggregationTime"], usecols=[0, 2])
        agg_data['Time'] = agg_data['Time'] / 1_000  # Convert time to seconds
        #agg_data['AggregationTime'] = agg_data['AggregationTime'] / 1_000  # Convert aggregation time to milliseconds

        # Plotting
        plt.figure(figsize=(12, 6))
        ax1 = plt.gca()  # Get current axis for Throughput
        ax2 = ax1.twinx()  # Create another y-axis for the aggregation time

        # Plot Throughput data for each flow on the left y-axis
        for key, df in throughput_data.items():
            ax1.plot(df['Time'], df['Throughput'], label=f'Throughput - {key}')

        # Plot Aggregation Time data on the right y-axis
        ax2.plot(agg_data['Time'], agg_data['AggregationTime'], label='Aggregation Time', color='red', linestyle='--')

        # Set labels and titles
        ax1.set_xlabel('Time (seconds)', fontsize=14)
        ax1.set_ylabel('Throughput (Mbps)', fontsize=14)
        ax2.set_ylabel('Aggregation Time (ms)', fontsize=14)
        ax1.set_title('Consumer: Throughput vs. Aggregation Time', fontsize=16)

        # Adding legends
        lines1, labels1 = ax1.get_legend_handles_labels()
        lines2, labels2 = ax2.get_legend_handles_labels()
        ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')

        ax1.grid(True)
        plt.tight_layout()

        # Save the plot to the specified output directory
        output_file_path = os.path.join(output_directory, "con_throughput_aggTime.png")
        plt.savefig(output_file_path, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file_path}")

    except Exception as e:
        print(f"An error occurred: {e}")

