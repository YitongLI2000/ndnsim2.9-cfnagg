import os
import pandas as pd
import matplotlib.pyplot as plt


def rto_plot(input_directory, output_directory):
    try:
        flow_data = {}

        # Iterate over files in the directory
        files = os.listdir(input_directory)

        # Filter and organize files by flows
        for file in files:
            if file.startswith("con0_RTO"):
                _, _, flow = file.replace(".txt", "").split("_")
                file_path = os.path.join(input_directory, file)

                # Read RTO data from the file
                data = pd.read_csv(file_path, sep=" ", header=None, names=["Time", "RTO"])

                # Units transformation
                #data['RTO'] /= 1_000  # Convert to milliseconds
                data['Time'] /= 1_000  # Convert to milliseconds

                flow_data[flow] = data

        # Plot data for each flow
        plt.figure(figsize=(12, 6))
        
        for key, df in flow_data.items():
            plt.plot(df['Time'], df['RTO'], label=f'{key}')

        plt.xlabel('Time (seconds)', fontsize=14)
        plt.ylabel('RTO (ms)', fontsize=14)
        plt.title("RTO - Consumer", fontsize=16)
        plt.legend(fontsize=12)
        plt.grid(True)
        plt.xticks(fontsize=14)
        plt.yticks(fontsize=14)
        plt.tight_layout()

        # Save the plot
        output_file = os.path.join(output_directory, "con_rto.png")
        print("Plot saved to:", output_file)
        plt.savefig(output_file, dpi=300)
        plt.close()

    except Exception as e:
        print(f"Error occurred: {e}")




