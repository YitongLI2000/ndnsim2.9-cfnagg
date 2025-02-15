import os
import pandas as pd
import matplotlib.pyplot as plt

def inFlight_plot(input_directory, output_directory):
    try:
        flow_data = {}

        # Iterate over files in the directory
        files = os.listdir(input_directory)

        # Filter and organize files by flows
        for file in files:
            if file.startswith("con0_queue"):
                _, _, flow = file.replace(".txt", "").split("_")
                file_path = os.path.join(input_directory, file)

                # Read inFlight data from the file
                data = pd.read_csv(file_path, sep="\s+", header=None, names=["Time", "InFlight"], usecols=[0, 5])

                # Debug print to check the columns
                #print(f"Columns in {file}: {data.columns}")

                # Units transformation
                #data['inFlight'] /= 1_000  # Convert to milliseconds
                data['Time'] /= 1_000  # Convert to seconds

                flow_data[flow] = data

        # Plot data for each flow
        plt.figure(figsize=(12, 6))
        
        for key, df in flow_data.items():
            plt.plot(df['Time'], df['InFlight'], label=f'{key}')

        plt.xlabel('Time (seconds)', fontsize=14)
        plt.ylabel('inFlight (packets)', fontsize=14)
        plt.title("inFlight - Consumer", fontsize=16)
        plt.legend(fontsize=12)
        plt.grid(True)
        plt.xticks(fontsize=14)
        plt.yticks(fontsize=14)
        plt.tight_layout()

        # Save the plot
        output_file = os.path.join(output_directory, "con_inflight.png")
        plt.savefig(output_file, dpi=300)
        plt.close()
        print(f"Plot saved to: {output_file}")

    except Exception as e:
        print(f"Error occurred: {e}")