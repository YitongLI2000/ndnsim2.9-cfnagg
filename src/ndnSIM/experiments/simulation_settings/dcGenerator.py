import os
import sys
import re
import configparser
import math


def is_valid_parameter(input_param):
    """
    Check whether input parameter is int
    :param input_param:
    :return:
    """
    try:
        int(input_param)
        return True
    except ValueError:
        return False


def is_valid_bitrate(bitrate):
    """
    Check whether input bitrate is in the correct format
    :param bitrate: Input bitrate
    :return:
    """
    pattern = re.compile(r"^\d+(Kbps|Mbps|Gbps|bps)$")
    return pattern.match(bitrate) is not None


def is_valid_delay(delay):
    """
    Check whether input delay is in the correct format
    :param delay: Input delay (ms)
    :return:
    """
    pattern = re.compile(r"^\d+ms$")
    return pattern.match(delay)



def generate_topology(num_producers, num_producers_per_rack_type1, num_racks_type2, bit_rate, link_cost, delay, queue):
    """
    Generate a new DC topology based on the defined rack types and connection rules.
    
    :param num_producers: Total number of producers in all type 1 racks
    :param num_aggregators: Total number of aggregators across all racks
    :param num_producers_per_rack_type1: Number of producers in each type 1 rack
    :param num_racks_type2: Number of type 2 racks
    :return: None
    """
    # Calculate the number of Type 1 racks needed
    num_racks_type1 = (num_producers + num_producers_per_rack_type1 - 1) // num_producers_per_rack_type1

    with open("../../examples/topologies/DataCenterTopology.txt", "w") as output_file:
        output_file.write("router\n\n")
        
        # Counters for global indexing
        con_index = 0
        pro_index = 0
        agg_index = 0
        forwarder_index = 0
        
        # Type 1 racks
        remaining_producers = num_producers
        for i in range(num_racks_type1):
            is_special_rack = (i == 0)  # Assume the first rack is special
            num_producers_in_rack = min(num_producers_per_rack_type1, remaining_producers)
            remaining_producers -= num_producers_in_rack

            for p in range(num_producers_in_rack):
                output_file.write(f"pro{pro_index}\n")
                pro_index += 1
            if is_special_rack:
                output_file.write(f"con{con_index}\n")
                con_index += 1
            output_file.write(f"agg{agg_index}\n")
            agg_index += 1
            output_file.write(f"forwarder{forwarder_index}\n")
            forwarder_index += 1

        # Type 2 racks
        for i in range(num_racks_type2):
            output_file.write(f"agg{agg_index}\n")
            agg_index += 1
            output_file.write(f"forwarder{forwarder_index}\n")
            forwarder_index += 1

        # Links section
        output_file.write("\nlink\n\n")
        
        # Type 1 racks links
        base_pro_index = 0
        base_agg_index = 0
        base_forwarder_index = 0
        for i in range(num_racks_type1):
            is_special_rack = (i == 0)
            num_producers_in_rack = min(num_producers_per_rack_type1, num_producers - base_pro_index)

            for p in range(num_producers_in_rack):
                output_file.write(f"pro{base_pro_index}       forwarder{base_forwarder_index}       {bit_rate}       {link_cost}       {delay}       {queue}\n")
                base_pro_index += 1
            if is_special_rack:
                output_file.write(f"con0       forwarder{base_forwarder_index}       {bit_rate}       {link_cost}       {delay}       {queue}\n")
            output_file.write(f"agg{base_agg_index}       forwarder{base_forwarder_index}       {bit_rate}       {link_cost}       {delay}       {queue}\n")
            base_agg_index += 1
            base_forwarder_index += 1

        # Type 2 racks links
        for i in range(num_racks_type2):
            output_file.write(f"agg{base_agg_index}       forwarder{base_forwarder_index}       {bit_rate}       {link_cost}       {delay}       {queue}\n")
            base_agg_index += 1
            base_forwarder_index += 1

        # Inter-rack links for Type 1 to Type 2
        for i in range(num_racks_type1):
            target_type2_rack = i % num_racks_type2
            output_file.write(f"forwarder{i}       forwarder{num_racks_type1 + target_type2_rack}       {bit_rate}       {link_cost}       {delay}       {queue}\n")

        # Fully connect all Type 2 racks
        for i in range(num_racks_type2):
            for j in range(i + 1, num_racks_type2):
                output_file.write(f"forwarder{num_racks_type1 + i}       forwarder{num_racks_type1 + j}       {bit_rate}       {link_cost}       {delay}       {queue}\n")




def main():
    # Read information from config.ini
    config = configparser.ConfigParser()
    config.read('config.ini')

    if not is_valid_parameter(config['DCNTopology']['numProducer']):
        print(f"Error! Num_producers '{config['DCNTopology']['numProducer']}' is not integer, please check.")
        sys.exit(1)

    if not is_valid_parameter(config['DCNTopology']['numAggregator']):
        print(f"Error! Num_aggregators '{config['DCNTopology']['numAggregator']}' is not int, please check.")
        sys.exit(1)

    if not is_valid_parameter(config['DCNTopology']['numProducerPerRack']):
        print(f"Error! Number_producer_per_edge '{config['DCNTopology']['numProducerPerRack']}' is not int, please check.")
        sys.exit(1)   

    if not is_valid_bitrate(config['DCNTopology']['Bitrate']):
        print(f"Error! Bitrate '{config['DCNTopology']['Bitrate']}' is not in the correct format. Only the following format is allowed:\n1bps, 1Kbps, 1Mbps, 1Gbps")
        sys.exit(1)

    if not is_valid_parameter(config['DCNTopology']['linkCost']):
        print(f"Error! Link_cost '{config['DCNTopology']['linkCost']}' is not int, please check.")
        sys.exit(1)          

    if not is_valid_delay(config['DCNTopology']['propagationDelay']):
        print(f"Error! Propagation delay '{config['DCNTopology']['propagationDelay']}' is not in the correct format. Only the following format is allowed:\n1ms")
        sys.exit(1)

    if not is_valid_parameter(config['DCNTopology']['queue']):
        print(f"Error! Queue '{config['DCNTopology']['queue']}' is not int, please check.")
        sys.exit(1)           

    num_producers = int(config['DCNTopology']['numProducer'])
    num_aggregators = int(config['DCNTopology']['numAggregator'])
    num_producers_per_rack_type1 = int(config['DCNTopology']['numProducerPerRack'])
    num_racks_type2 = num_aggregators - math.ceil(num_producers/num_producers_per_rack_type1)

    bit_rate = config['DCNTopology']['Bitrate']
    link_cost = int(config['DCNTopology']['linkCost'])
    propagation_delay = config['DCNTopology']['propagationDelay']
    queue = int(config['DCNTopology']['queue'])

    #print("Current Working Directory:", os.getcwd())

    generate_topology(num_producers, num_producers_per_rack_type1, num_racks_type2, bit_rate, link_cost, propagation_delay, queue)

    print("Topology generated successfully!")


if __name__ == "__main__":
    main()
