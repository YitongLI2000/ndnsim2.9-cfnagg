import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_queue_and_agg_time(input_directory, output_directory):
    try:
        # Initialize dictionaries to store data from each aggregator and flow
        queue_data = {}

        # Iterate over files in the input directory
        files = os.listdir(input_directory)
        for file in files:
            if file.endswith(".txt") and "queue" in file:
                parts = file.split('_')
                aggregator = parts[0]  # Extract the aggregator identifier
                flow = parts[2].split('.')[0]  # Extract the flow identifier

                file_path = os.path.join(input_directory, file)

                # Read queue data from the file
                data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "Queue"], usecols=[0, 4])
                data['Time'] = data['Time'] / 1_000  # Convert time to seconds for uniformity

                # Use a tuple (aggregator, flow) as the key to handle multiple aggregators and flows
                queue_data[(aggregator, flow)] = data

        # Process aggregation time data for each aggregator
        for file in files:
            if file.endswith("aggregationTime.txt"):
                aggregator = file.split('_')[0]
                agg_time_file = os.path.join(input_directory, file)

                # Read the single aggregation time data file
                agg_data = pd.read_csv(agg_time_file, sep="\s+", header=None, names=["Time", "AggregationTime"], usecols=[0, 2])
                agg_data['Time'] = agg_data['Time'] / 1_000  # Convert time to seconds
                #agg_data['AggregationTime'] = agg_data['AggregationTime'] / 1_000  # Convert aggregation time to seconds

                # Plotting for each aggregator
                plt.figure(figsize=(12, 6))
                ax1 = plt.gca()  # Get current axis for queue
                ax2 = ax1.twinx()  # Create another y-axis for the aggregation time

                # Plot queue data for each flow on the left y-axis
                for key, df in queue_data.items():
                    if key[0] == aggregator:  # Check if the data belongs to the current aggregator
                        # Add line plot for queue
                        ax1.plot(df['Time'], df['Queue'], label=f'Queue - {key[1]}', alpha=0.7)
                        # Add scatter points to emphasize queue data points
                        #ax1.scatter(df['Time'], df['Queue'], s=10, label=f'Queue Points - {key[1]}', alpha=0.6)

                # Plot Aggregation Time data on the right y-axis
                ax2.plot(agg_data['Time'], agg_data['AggregationTime'], label='Aggregation Time', color='red', linestyle='--', alpha=0.7)
                # Add scatter points to emphasize Aggregation Time data points
                #ax2.scatter(agg_data['Time'], agg_data['AggregationTime'], s=10, color='red', label='Aggregation Time Points', alpha=0.6)

                # Set labels and titles
                ax1.set_xlabel('Time (seconds)', fontsize=14)
                ax1.set_ylabel('Queue (packets)', fontsize=14)
                ax2.set_ylabel('Aggregation Time (ms)', fontsize=14)
                ax1.set_title(f'Queue vs. Aggregation Time for {aggregator}', fontsize=16)

                # Adding legends
                lines1, labels1 = ax1.get_legend_handles_labels()
                lines2, labels2 = ax2.get_legend_handles_labels()
                ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')

                ax1.grid(True)
                plt.tight_layout()

                # Save the plot to the specified output directory
                output_file_path = os.path.join(output_directory, f"{aggregator}_queue_aggTime.png")
                plt.savefig(output_file_path, dpi=300)
                plt.close()
                print(f"Plot saved to: {output_file_path}")

    except Exception as e:
        print(f"An error occurred: {e}")