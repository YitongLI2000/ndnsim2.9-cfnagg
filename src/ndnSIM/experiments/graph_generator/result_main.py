import sys
import os
from utils.utility import *

#import consumer.con_cwnd as con_cwnd
#import consumer.con_rtt_cwnd as con_rtt_cwnd
#import consumer.con_inFlight_cwnd as con_inFlight_cwnd
#import consumer.con_qsf_aggTime as con_qsf_aggTime
#import consumer.con_rtt_qsf as con_rtt_qsf
#import consumer.con_throughput_qsf as con_throughput_qsf
import consumer.con_rto as con_rto
import consumer.con_rtt as con_rtt
import consumer.con_rtt_aggTime as con_rtt_aggTime
import consumer.con_rtt_rto as con_rtt_rto
import consumer.con_sendRate_BW as con_sendRate_BW
import consumer.con_sendRate_throughput as con_sendRate_throughput
import consumer.con_throughput_aggTime as con_throughput_aggTime
import consumer.con_inFlight as con_inFlight
import consumer.con_queue_aggTime as con_queue_aggTime
import consumer.con_rtt_queue as con_rtt_queue
import consumer.con_sendRate_inFlight as con_sendRate_inFlight
import consumer.con_sendRate_queue as con_sendRate_queue
import consumer.con_throughput_queue as con_throughput_queue
import consumer.con_rtt_inFlight as con_rtt_inFlight

#import aggregator.agg_cwnd as agg_cwnd
#import aggregator.agg_inFlight_cwnd as agg_inFlight_cwnd
#import aggregator.agg_qsf_queue as agg_qsf_queue
#import aggregator.agg_rtt_qsf as agg_rtt_qsf
#import aggregator.agg_qsf_aggTime as agg_qsf_aggTime
#import aggregator.agg_throughput_qsf as agg_throughput_qsf
import aggregator.agg_rtt as agg_rtt
import aggregator.agg_rto as agg_rto
import aggregator.agg_rtt_aggTime as agg_rtt_aggTime
import aggregator.agg_sendRate_BW as agg_sendRate_BW
import aggregator.agg_sendRate_throughput as agg_sendRate_throughput
import aggregator.agg_sendRate as agg_sendRate
import aggregator.agg_throughput as agg_throughput
import aggregator.agg_throughput_aggTime as agg_throughput_aggTime
import aggregator.agg_inFlight as agg_inFlight
import aggregator.agg_rtt_queue as agg_rtt_queue
import aggregator.agg_queue_aggTime as agg_queue_aggTime
import aggregator.agg_sendRate_queue as agg_sendRate_queue
import aggregator.agg_throughput_queue as agg_throughput_queue
import aggregator.agg_sendRate_inFlight as agg_sendRate_inFlight
import aggregator.agg_rtt_inflight as agg_rtt_inflight

import forwarder.fwd_throughput as fwd_throughput


def main():
    try:
        log_con = "../../results/logs/con"
        log_agg = "../../results/logs/agg"
        log_fwd = "../../results/logs/fwd"

        # Check if current working directory exists, if not, then create it
        if os.path.exists(log_con) and os.path.isdir(log_con) and os.path.exists(log_agg) and os.path.isdir(log_agg) and os.path.exists(log_fwd) and os.path.isdir(log_fwd):
            print("Input directory exists.")
        else:
            print("Input directory does not exist, please check!")
            sys.exit(1)

        # Check if the output directory exists, if not, then create it
        output_con = "../../results/graphs/con"
        output_agg = "../../results/graphs/agg"
        output_fwd = "../../results/graphs/fwd"
        check_and_create_dir(output_con)
        check_and_create_dir(output_agg)
        check_and_create_dir(output_fwd)

        # Generate consumer's graphs
        #con_cwnd.cwnd_plot(log_con, output_con)
        #con_rtt_cwnd.plot_rtt_and_cwnd(log_con, output_con)
        #con_inFlight_cwnd.plot_inFlight_and_cwnd(log_con, output_con)
        #con_qsf_aggTime.plot_qsf_and_agg_time(log_con, output_con)
        #con_rtt_qsf.plot_rtt_and_qsf(log_con, output_con)   
        #con_throughput_qsf.plot_throughput_and_qsf(log_con, output_con)             
        con_rto.rto_plot(log_con, output_con)
        con_rtt.rtt_plot(log_con, output_con)
        con_rtt_aggTime.plot_rtt_and_agg_time(log_con, output_con)
        con_rtt_rto.plot_rtt_and_rto(log_con, output_con)
        con_sendRate_BW.plot_sendrate_and_bw(log_con, output_con)
        con_sendRate_throughput.plot_sendrate_and_throughput(log_con, output_con)
        con_throughput_aggTime.plot_throughput_and_agg_time(log_con, output_con)
        con_inFlight.inFlight_plot(log_con, output_con)
        con_queue_aggTime.plot_queue_and_agg_time(log_con, output_con)
        con_rtt_queue.plot_rtt_and_queue(log_con, output_con)
        con_rtt_queue.plot_combined_rtt_and_queue(log_con, output_con)
        con_sendRate_inFlight.plot_sendrate_and_inflight(log_con, output_con)
        con_sendRate_inFlight.plot_combined_sendrate_and_inflight(log_con, output_con)
        con_sendRate_queue.plot_sendrate_and_queue(log_con, output_con)
        con_sendRate_queue.plot_combined_sendrate_and_queue(log_con, output_con)
        con_throughput_queue.plot_throughput_and_queue(log_con, output_con)
        con_throughput_queue.plot_combined_throughput_and_queue(log_con, output_con)
        con_rtt_inFlight.plot_rtt_and_inflight(log_con, output_con)
        con_rtt_inFlight.plot_combined_rtt_and_inflight(log_con, output_con)

        # Generate aggregator's graphs
        #agg_inFlight_cwnd.plot_inFlight_and_cwnd(log_agg, output_agg)
        #agg_cwnd.plot_cwnd(log_agg, output_agg)        
        #agg_qsf_queue.plot_qsf_and_queue(log_agg, output_agg)
        #agg_rtt_qsf.plot_rtt_and_qsf(log_agg, output_agg)
        #agg_qsf_aggTime.plot_qsf_and_agg_time(log_agg, output_agg)
        #agg_throughput_qsf.plot_throughput_and_qsf(log_agg, output_agg)
        #agg_throughput_qsf.plot_combined_throughput_and_qsf(log_agg, output_agg)
        agg_rtt.plot_rtt(log_agg, output_agg)
        agg_rto.plot_rto(log_agg, output_agg)
        agg_rtt_aggTime.plot_rtt_and_agg_time(log_agg, output_agg)
        agg_sendRate_BW.plot_sendRate_and_bw(log_agg, output_agg)
        agg_sendRate_throughput.plot_sendRate_and_throughput(log_agg, output_agg)
        agg_sendRate.plot_sendRate(log_agg, output_agg)
        agg_throughput.plot_throughput(log_agg, output_agg)
        agg_throughput_aggTime.plot_throughput_and_agg_time(log_agg, output_agg)
        agg_inFlight.plot_inFlight(log_agg, output_agg)
        agg_queue_aggTime.plot_queue_and_agg_time(log_agg, output_agg)
        agg_throughput_queue.plot_combined_throughput_and_queue(log_agg, output_agg)
        agg_throughput_queue.plot_throughput_and_queue(log_agg, output_agg)
        agg_sendRate_inFlight.plot_sendRate_and_inflight(log_agg, output_agg)
        agg_sendRate_inFlight.plot_combined_sendRate_and_inflight(log_agg, output_agg)
        agg_rtt_queue.plot_rtt_and_queue(log_agg, output_agg)
        agg_rtt_queue.plot_combined_rtt_and_queue(log_agg, output_agg)
        agg_sendRate_queue.plot_sendRate_and_queue(log_agg, output_agg)
        agg_sendRate_queue.plot_combined_sendRate_and_queue(log_agg, output_agg)
        agg_rtt_inflight.plot_rtt_and_inflight(log_agg, output_agg)
        agg_rtt_inflight.plot_combined_rtt_and_inflight(log_agg, output_agg)

        # Generate forwarder's graphs
        fwd_throughput.plot_throughput(log_fwd, output_fwd)

    except Exception as e:
        print(f"Error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()