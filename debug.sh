#!/bin/bash

GRAPH_DIR="./src/ndnSIM/results/graphs"
LOGS_DIR="./src/ndnSIM/results/logs"
PACKET_CAP_DIR="./src/ndnSIM/results/packet_traces"

# Define function to clear all contents from specific path
clear_directory() {
    local directory=$1

    # Check if directory exists
    if [ -d "$directory" ]; then
        # Remove all contents within the directory
        echo "Removing all contents in $directory"
        rm -rf $directory/*

        # Check if the removal was successful
        if [ $? -eq 0 ]; then
            echo "All contents have been removed successfully from $directory."
        else
            echo "Failed to remove contents from $directory."
        fi
    else
        echo "Directory does not exist: $directory, no need for further operation."
    fi
}

# Remove everything from specific folders
clear_directory "$GRAPH_DIR"
clear_directory "$LOGS_DIR"
clear_directory "$PACKET_CAP_DIR"

# Specify python version
PYTHON_VERSION="python3.10"

# Use the provided argument to find the Python executable path
PYTHON=$(which "$PYTHON_VERSION")

# Check if the Python executable was found
if [ -z "$PYTHON" ]; then
  echo "Error: Python executable not found for version $PYTHON_VERSION"
  exit 1
fi

# Generate corresponding network topology, DCN
cd ./src/ndnSIM/experiments/simulation_settings
CONFIG_FILE="config.ini"
TOPOLOGY_TYPE=$(grep "TopologyType" $CONFIG_FILE | awk -F "= " '{print $2}')

if [ "$TOPOLOGY_TYPE" == "DCN" ]; then
    echo "TopologyType is DCN. Generating DC network..."
    $PYTHON dcGenerator.py
    TOPOLOGY_FILE="./src/ndnSIM/examples/topologies/DCNTopology.txt"
elif [ "$TOPOLOGY_TYPE" == "ISP" ]; then
    echo "Using existing ISP topology..."
    #$PYTHON ispGenerator.py
    TOPOLOGY_FILE="./src/ndnSIM/examples/topologies/ISPTopology.txt"
elif [ "$TOPOLOGY_TYPE" == "BinaryTree8" ]; then
    echo "Running with testing 8 pro binary tree topology..."
    TOPOLOGY_FILE="./src/ndnSIM/examples/topologies/BinaryTreeTopology8.txt"    
elif [ "$TOPOLOGY_TYPE" == "BinaryTree16" ]; then
    echo "Running with testing 16 pro binary tree topology..."
    TOPOLOGY_FILE="./src/ndnSIM/examples/topologies/BinaryTreeTopology16.txt"    
elif [ "$TOPOLOGY_TYPE" == "BinaryTree32" ]; then
    echo "Running with testing 32 pro binary tree topology..."
    TOPOLOGY_FILE="./src/ndnSIM/examples/topologies/BinaryTreeTopology32.txt"         
else
    echo "TopologyType can't be recognized."
    exit 1
fi

# Check whether topology is generated successfully
if [ $? -ne 0 ]; then
    echo "Fail to generate topology, please check all input parameters!"
    exit 1
fi

cd ../../../../

# Start simulation, currently under "ns-3"
#NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator:ndn.Producer ./waf --run cfnagg
#NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run cfnagg
#NS_LOG=ndn.Consumer:ndn.ConsumerINA ./waf --run cfnagg
#NS_LOG=ndn.Aggregator ./waf --run cfnagg
#./waf --run cfnagg
#NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run variate-bw
#NS_LOG=ndn-cxx.nfd.Strategy ./waf --run cfnagg
#NS_LOG=ndn-cxx.nfd.BestRouteStrategy:ndn-cxx.nfd.Forwarder:ndn.Consumer:ndn.Aggregator ./waf --run cfnagg
NS_LOG=ndn-cxx.nfd.BestRouteStrategy:ndn-cxx.nfd.Forwarder:ndn.Consumer:ndn.Aggregator ./waf --run cfnagg > log.txt 2>&1
#NS_LOG=ndn-cxx.nfd.FibManager ./waf --run cfnagg
#NS_LOG=ndn.GlobalRoutingHelper ./waf --run cfnagg


# Copy config.ini to logs directory
cp ./src/ndnSIM/experiments/simulation_settings/config.ini $LOGS_DIR
if [ $? -eq 0 ]; then
    echo "config.ini has been copied to $LOGS_DIR successfully."
else
    echo "Failed to copy config.ini to $LOGS_DIR."
    exit 1
fi


# Copy the correct topology file based on TopologyType
if [ -f "$TOPOLOGY_FILE" ]; then
    cp "$TOPOLOGY_FILE" "$LOGS_DIR/"
    if [ $? -eq 0 ]; then
        echo "Topology file ($TOPOLOGY_FILE) has been copied to $LOGS_DIR successfully."
    else
        echo "Failed to copy topology file ($TOPOLOGY_FILE) to $LOGS_DIR."
        exit 1
    fi
else
    echo "Error: Topology file ($TOPOLOGY_FILE) does not exist!"
    exit 1
fi


# Run topo_visualization.py to generate the topology visualization
#echo "Running topo_visualization.py to generate topology visualization..."
#cd ./src/ndnSIM/experiments/graph_generator
#$PYTHON ./src/ndnSIM/experiments/graph_generator/topo_visualization.py
#$PYTHON topo_visualization.py
#if [ $? -ne 0 ]; then
#    echo "Error: Topology visualization failed!"
#    exit 1
#else
#    echo "Topology visualization completed successfully."
#fi



# Generate simulation result graphs
cd ./src/ndnSIM/experiments/graph_generator
$PYTHON result_main.py
if [ $? -ne 0 ]; then
    echo "Error: Failed to generate all result graphs, please check again!"
    exit 1
fi
cd ../../../../

# Compute bandwidth utilization
#cd ../
#$PYTHON bandwidth_utilization.py
#if [ $? -ne 0 ]; then
#    echo "Fail to compute bandwidth utilization, please check again!"
#    exit 1
#fi

#$PYTHON throughput_measurement.py
#$PYTHON consumer_result_generator.py
#$PYTHON agg_result_generator.py
