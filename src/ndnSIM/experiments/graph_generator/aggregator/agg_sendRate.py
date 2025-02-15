import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_sendRate(input_directory, output_directory):
    try:
        file_prefix = "agg"
        aggregator_data = {}

        # Iterate over files in the directory
        files = os.listdir(input_directory)
        for file in files:
            if file.startswith(file_prefix):
                parts = file.split('_')
                if 'queue' in parts:
                    aggregator_index = parts[0][3:]  # Extract index right after 'agg'
                    flow_name = parts[2].split('.')[0]  # Extract 'pro'y' part, remove '.txt'
                    if aggregator_index not in aggregator_data:
                        aggregator_data[aggregator_index] = {}
                    file_path = os.path.join(input_directory, file)

                    # Read SendRate data from the file
                    data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "SendRate"], usecols=[0, 1])
                    data['Time'] = data['Time'] / 1_000  # Convert to seconds

                    aggregator_data[aggregator_index][flow_name] = data

        # Plot data for each aggregator
        for aggregator_index, data_dict in aggregator_data.items():
            # print(f"Plotting data for aggregator {aggregator_index}")
            plt.figure(figsize=(12, 6))
            
            for key, df in data_dict.items():
                plt.plot(df['Time'], df['SendRate'], label=f'Flow {key}')

            plt.xlabel('Time (seconds)', fontsize=14)
            plt.ylabel('SendRate (packets/ms)', fontsize=14)
            plt.title(f'SendRate - Aggregator {aggregator_index}', fontsize=16)
            plt.legend(loc='upper right', fontsize=8)
            plt.grid(True)
            plt.xticks(fontsize=14)
            plt.yticks(fontsize=14)
            plt.tight_layout()

            # Save the plot
            output_file = os.path.join(output_directory, f"agg{aggregator_index}_sendRate.png")
            plt.savefig(output_file, dpi=300)
            plt.close()
            print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"Error occurred: {e}")