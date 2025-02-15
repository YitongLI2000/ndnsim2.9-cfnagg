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


def generate_isp_topology(num_producers, num_aggregators, num_producers_per_forwarder, bitrate_access, bitrate_agg, bitrate_core, link_cost_access, link_cost_agg, link_cost_core, delay_access, delay_agg, delay_core, queue):
    """
    Generate a three-layer ISP network topology.
    
    :param num_producers: Total number of producers in the access layer
    :param num_aggregators: Total number of aggregators in the aggregation layer
    :param num_producers_per_forwarder: Number of producers connected to each forwarder in the access layer
    :param bitrate_access: Bitrate of the access layer links
    :param bitrate_agg: Bitrate of the aggregation layer links
    :param bitrate_core: Bitrate of the core layer links
    :param link_cost_access: Cost of the access layer links
    :param link_cost_agg: Cost of the aggregation layer links
    :param link_cost_core: Cost of the core layer links
    :param delay_access: Propagation delay of the access layer links
    :param delay_agg: Propagation delay of the aggregation layer links
    :param delay_core: Propagation delay of the core layer links
    :param queue: Queue size of the links
    :return: None
    """
    num_forwarders = (num_producers + num_producers_per_forwarder - 1) // num_producers_per_forwarder
    total_forwarders = num_forwarders + 5 + 1  # Additional 5 for core layer and 1 for consumer

    with open("../../examples/topologies/ISPTopology.txt", "w") as output_file:
        output_file.write("router\n\n")
        
        # Define routers at all layers
        for p in range(num_producers):
            output_file.write(f"pro{p}\n")
        output_file.write("con0\n")
        for f in range(total_forwarders):
            output_file.write(f"forwarder{f}\n")
        for a in range(num_aggregators):
            output_file.write(f"agg{a}\n")
        
        # Links section
        output_file.write("\nlink\n\n")
        
        # Access Layer Links
        base_pro_index = 0
        for f in range(num_forwarders):
            num_producers_in_forwarder = min(num_producers_per_forwarder, num_producers - base_pro_index)
            for p in range(num_producers_in_forwarder):
                output_file.write(f"pro{base_pro_index}       forwarder{f}       {bitrate_access}       {link_cost_access}       {delay_access}       {queue}\n")
                base_pro_index += 1
        
        # Consumer's connection to its individual forwarder (last forwarder in the list)
        # Currently change the bitrate from access to core!
        output_file.write(f"con0       forwarder{num_forwarders}       {bitrate_core}       {link_cost_access}       {delay_access}       {queue}\n")
        
        # Aggregation Layer Links
        base_forwarder_index = 0
        for a in range(num_aggregators):
            # Connect aggregator to one core forwarder (starting from num_forwarders + 1)
            core_forwarder_index = num_forwarders + 1 + (a % 5)
            output_file.write(f"forwarder{base_forwarder_index}       agg{a}       {bitrate_agg}       {link_cost_agg}       {delay_agg}       {queue}\n")
            output_file.write(f"agg{a}       forwarder{core_forwarder_index}       {bitrate_agg}       {link_cost_agg}       {delay_agg}       {queue}\n")
            base_forwarder_index = (base_forwarder_index + 1) % num_forwarders
        
        # Connection of consumer's forwarder to one core forwarder (first core forwarder)
        output_file.write(f"forwarder{num_forwarders}       forwarder{num_forwarders + 1}       {bitrate_core}       {link_cost_core}       {delay_core}       {queue}\n")
        
        # Fully connect all core forwarders
        for i in range(5):
            for j in range(i + 1, 5):
                output_file.write(f"forwarder{num_forwarders + 1 + i}       forwarder{num_forwarders + 1 + j}       {bitrate_core}       {link_cost_core}       {delay_core}       {queue}\n")


def main():
    # num_producers, num_aggregators, num_producers_per_forwarder, bitrate_access, bitrate_agg, bitrate_core, link_cost_access, link_cost_agg, link_cost_core, delay_access, delay_agg, delay_core, queue

    # Read information from config.ini
    config = configparser.ConfigParser()
    config.read('config.ini')

    if not is_valid_parameter(config['ISPTopology']['numProducer']):
        print(f"Error! Num_producers '{config['ISPTopology']['numProducer']}' is not integer, please check.")
        sys.exit(1)

    if not is_valid_parameter(config['ISPTopology']['numAggregator']):
        print(f"Error! Num_aggregators '{config['ISPTopology']['numAggregator']}' is not int, please check.")
        sys.exit(1)

    if not is_valid_parameter(config['ISPTopology']['numProducerPerForwarder']):
        print(f"Error! Number_producer_per_forwarder '{config['ISPTopology']['numProducerPerForwarder']}' is not int, please check.")
        sys.exit(1)   

    if not is_valid_bitrate(config['ISPTopology']['bitrate_access']):
        print(f"Error! Bitrate '{config['ISPTopology']['bitrate_access']}' is not in the correct format. Only the following format is allowed:\n1bps, 1Kbps, 1Mbps, 1Gbps")
        sys.exit(1)

    if not is_valid_bitrate(config['ISPTopology']['bitrate_agg']):
        print(f"Error! Bitrate '{config['ISPTopology']['bitrate_agg']}' is not in the correct format. Only the following format is allowed:\n1bps, 1Kbps, 1Mbps, 1Gbps")
        sys.exit(1)

    if not is_valid_bitrate(config['ISPTopology']['bitrate_core']):
        print(f"Error! Bitrate '{config['ISPTopology']['bitrate_core']}' is not in the correct format. Only the following format is allowed:\n1bps, 1Kbps, 1Mbps, 1Gbps")
        sys.exit(1)

    if not is_valid_parameter(config['ISPTopology']['link_cost_access']):
        print(f"Error! Link_cost '{config['ISPTopology']['link_cost_access']}' is not int, please check.")
        sys.exit(1)          

    if not is_valid_parameter(config['ISPTopology']['link_cost_agg']):
        print(f"Error! Link_cost '{config['ISPTopology']['link_cost_agg']}' is not int, please check.")
        sys.exit(1) 

    if not is_valid_parameter(config['ISPTopology']['link_cost_core']):
        print(f"Error! Link_cost '{config['ISPTopology']['link_cost_core']}' is not int, please check.")
        sys.exit(1) 

    if not is_valid_delay(config['ISPTopology']['delay_access']):
        print(f"Error! Propagation delay '{config['ISPTopology']['delay_access']}' is not in the correct format. Only the following format is allowed:\n1ms")
        sys.exit(1)

    if not is_valid_delay(config['ISPTopology']['delay_agg']):
        print(f"Error! Propagation delay '{config['ISPTopology']['delay_agg']}' is not in the correct format. Only the following format is allowed:\n1ms")
        sys.exit(1)

    if not is_valid_delay(config['ISPTopology']['delay_core']):
        print(f"Error! Propagation delay '{config['ISPTopology']['delay_core']}' is not in the correct format. Only the following format is allowed:\n1ms")
        sys.exit(1)

    if not is_valid_parameter(config['ISPTopology']['queue']):
        print(f"Error! Queue '{config['ISPTopology']['queue']}' is not int, please check.")
        sys.exit(1)           

    num_producers = int(config['ISPTopology']['numProducer'])
    num_aggregators = int(config['ISPTopology']['numAggregator'])
    num_producers_per_forwarder = int(config['ISPTopology']['numProducerPerForwarder'])
    bit_rate_access = config['ISPTopology']['bitrate_access']
    bit_rate_agg = config['ISPTopology']['bitrate_agg']
    bit_rate_core = config['ISPTopology']['bitrate_core']
    link_cost_access = int(config['ISPTopology']['link_cost_access'])
    link_cost_agg = int(config['ISPTopology']['link_cost_agg'])
    link_cost_core = int(config['ISPTopology']['link_cost_core'])
    delay_access = config['ISPTopology']['delay_access']
    delay_agg = config['ISPTopology']['delay_agg']
    delay_core = config['ISPTopology']['delay_core']
    queue = int(config['ISPTopology']['queue'])

    generate_isp_topology(num_producers, num_aggregators, num_producers_per_forwarder, bit_rate_access, bit_rate_agg, bit_rate_core, link_cost_access, link_cost_agg, link_cost_core, delay_access, delay_agg, delay_core, queue)

    print("Topology generated successfully!")


if __name__ == "__main__":
    main()