/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_CONSUMER_H
#define NDN_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"


#include "sliding-window.hpp"
#include "ndn-app.hpp"
#include "ModelData.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-value.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"
#include "ns3/ptr.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <iostream>
#include <sstream>
#include <queue>
#include <utility>
#include <deque>
#include <functional>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {


/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class Consumer : public App {
public:

    /**
     * Constructor
     */
    Consumer();
    virtual ~Consumer(){};


    /**
     * Initiate attributes for consumer class, some of them may be used, some are optional
     * Note that currently only use "NodePrefix", ignore "Prefix" for now
     *
     * @return Total TypeId
     */
    static TypeId GetTypeId();


    /**
    * Consumer sends relevant aggregation tree to all aggregators
    */
    void TreeBroadcast();

    
    /**
     * Implement the algorithm to compute aggregation tree
    */
    void ConstructAggregationTree();


    /**
     * Process returned data packets
     * @param data
     */
    virtual void
    OnData(shared_ptr<const Data> contentObject);


    /**
     * Invoked when Nack is triggered
     * @param nack
     */
    virtual void
    OnNack(shared_ptr<const lp::Nack> nack);

    /**
     * Triggered when timeout is triggered, timeout is traced using unique interest/data name
     * @param nameString
     */
    virtual void
    OnTimeout(std::string nameString);

    /**
     * This function generates complete interest name, by combining name and seq together, then call SendInterest to send it
     * Need to know whether it's the start of a new iteration
     * @param prefix
     */
    void
    SendPacket(std::string prefix);

    /**
     * Generate interests for all iterations and push them into queue for further interest sending scheduling
     * First invoke when start simulation, every time when one iteration finished aggregation, update interestQueue again
     * The number of interests of each round are checked, if any of them is not enough, generate interests into the queue
     */
    void
    InterestGenerator();

    /**
     * Split interest into new ones and store them into each flow's queue. 
     * If each flow's queue has at least one more space, split and return true; if any one of them is full, return false
     */
    bool
    InterestSplitting();

    /**
     * Perform aggregation (plus all model parameters together) operation, perform average when this iteration is done
     * @param data
     * @param seq
     */
    void Aggregate(const ModelData& data, const uint32_t& seq);


    /**
     * Get mean average of model parameters for one iteration
     * @param seq
     * @return Mean average of model parameters
     */
    std::vector<double> getMean(const uint32_t& seq);

    /**
     * Compute the total response time for average computation later
     * @param response_time
     */
    void ResponseTimeSum (int64_t response_time);

    /**
     * Compute average response time
     * @return Average response time
     */ 
    int64_t
    GetResponseTimeAverage();

    /**
     * Compute total aggregation time for all iterations
     * @param aggregate_time
     */
    void
    AggregateTimeSum (int64_t response_time);

    /**
     * Return average aggregation time in ms
     * @return Average aggregation time in certain iterations
     */
    int64_t
    GetAggregateTimeAverage();

    //? Utility function
    
    /**
     * Get data queue of certain flow
     * @param prefix flow name
     * @return Data queue size
     */
    double 
    getDataQueueSize(std::string prefix);

    //! Updated QueueSize-based CC

    /**
     * Based on returned data, update rtt estimation
     */
    void
    RTTMeasure(std::string prefix, int64_t resTime);


    /**
     * Get the data rate and return with correct value from the sliding window
     * @param prefix flow name
     * @return Data rate
     */
    double
    GetDataRate(std::string prefix);


    /**
     * Bandwidth estimation
     * @param prefix flow
     * @param dataArrivalRate data arrival rate
     */
    void 
    BandwidthEstimation(std::string prefix);


    /**
     * Update each flow's rate limit.
     * @param prefix flow name
     */
    void
    RateLimitUpdate(std::string prefix);


    /**
     * Detection of congestion based on RTT
     * @param responseTime
     * @param index round index
     * @return Whether congestion is detected
     */
    bool 
    CongestionDetection(std::string prefix, int64_t responseTime);

  
    /**
     * Compute new RTO based on response time of recent packets
     * @param resTime
     * @param roundIndex data packet's round index
     * @return New RTO
     */
    void 
    RTOMeasure(int64_t resTime, std::string prefix);

    /**
     * Record the RTT when receiving data packet
     */
    void 
    RTORecorder(std::string prefix);

    /**
     * Record in-flight packets when receiving a new packet
     */
    void 
    InFlightRecorder(std::string prefix);

    /**
     * Record the response time for each returned packet within corresponding files (per-flow CC)
     * @param responseTime
     * @param seq sequence number
     * @param ECN Whether ECN exist in current packet, type is boolean
     */
    void 
    ResponseTimeRecorder(std::string prefix, uint32_t seq, Time responseTime);


    /**
     * Record the aggregate time when each iteration finished
     * @param aggregateTime
     */
    void 
    AggregateTimeRecorder(Time aggregateTime, uint32_t seq);

    /**
     * Initialize all new log files, called in the beginning of simulation
     */
    void 
    InitializeLogFile();


    /**
     * Initialize all parameters for consumer class
     */
    void 
    InitializeParameter();

    /**
     * Check whether cwnd has been decreased within the last RTT duration
     * @return
     */
    bool 
    CanDecreaseWindow(std::string prefix, int64_t threshold);

    /**
     * Record the final throughput into file at the end of simulation
     * @param interestThroughput
     * @param dataThroughput
     */
    void 
    ThroughputRecorder(int interestThroughput, int dataThroughput, Time start_simulation, Time start_throughput);


    /**
     * Record the constructed aggregation tree into file
     * It's done when receiving "Initialization" data
     */
    void 
    AggTreeRecorder();


    /**
     * Record final simulation results
     */
    void 
    ResultRecorder(uint32_t iteNum, int timeoutNum, int64_t aveAggTime, int64_t totalTime);


    /**
     * Record queue size based CC info
     */
    void
    QueueRecorder(std::string prefix, double queueSize);

public:
    typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
    typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

protected:
    
    /**
     * Originally defined in ndn::App class, override here. Start the running process of consumer class
     */
    virtual void
    StartApplication();


    /**
     * Originally defined in ndn::App class, override here. Stop the running process of consumer class
     */
    virtual void
    StopApplication();

    virtual void
    ScheduleNextPacket(std::string prefix) = 0;


    /**
     * Called in SendPacket() function, construct interest packet and send it actually
     * @param newName
     */
    virtual void
    SendInterest(shared_ptr<Name> newName);

    /**
     * When invoked, check whether timeout happened for all packets in timeout list
     */
    void
    CheckRetxTimeout();

    /**
     * Set initial interval on how long to check timeout
     * @param retxTimer
     */
    void
    SetRetxTimer(Time retxTimer);

    /**
     * Get timer for timeout check
     * @return Timer set for timeout checking
     */
    Time
    GetRetxTimer() const;

public:
    /**
     * Get child nodes for given map and parent node. This function is originally defined in ndn::App class
     * @param key
     * @param treeMap
     * @return A map consisting all child nodes of the parent node
     */
    std::map<std::string, std::set<std::string>>
    getLeafNodes(const std::string& key,const std::map<std::string, std::vector<std::string>>& treeMap);

    /**
     * Return round index based on prefix
     * @param target
     * @return Round index
     */
    int findRoundIndex(const std::string& target);



protected:
    // Topology file name
    std::string filename;

    // Log path
    std::string conFolderPath = "src/ndnSIM/results/logs/con";
    std::string fwdFolderPath = "src/ndnSIM/results/logs/fwd";
    std::map<std::string, std::string> RTO_recorder;
    std::map<std::string, std::string> responseTime_recorder; // Format: 'Time', 'seq', 'RTT', ‘ECN’, 'isWindowDecreaseSuppressed'
    std::map<std::string, std::string> windowRecorder; // Format: 'Time', 'cwnd', 'cwnd threshold' (when cwnd is larger than the threshold, cwnd increase will be suppressed)
    std::map<std::string, std::string> inFlight_recorder; // Format: 'Time', 'inFlight'
    std::map<std::string, std::string> qsNew_recorder; //! Updated queue size CC
    std::string aggregateTime_recorder; // Format: 'Time', 'aggTime'
    int suspiciousPacketCount; // When timeout is triggered, add one
    int dataOverflow; // Record the number of data overflow
    int nackCount; // Record the number of NACK

    // Update when WindowDecrease() is called every time, used for CWA algorithm
    std::map<std::string, Time> lastWindowDecreaseTime;
    bool isWindowDecreaseSuppressed;

    // Throughput measurement
    int totalInterestThroughput;
    int totalDataThroughput;
    Time startSimulation;
    Time stopSimulation;
    Time startThroughputMeasurement;
    bool throughputStable; // Start record the throughput after first reach congestion, consider the point as the start of stable throughput


    // General window design
    uint32_t m_initialWindow;
    uint32_t m_minWindow;
    std::map<std::string, double> m_window;
    std::map<std::string, uint32_t> m_inFlight;

    // AIMD design
    std::map<std::string, double> m_ssthresh;
    bool m_useCwa;
    uint32_t m_highData;
    double m_recPoint;
    double m_alpha; // Timeout decrease factor
    double m_beta; // Local congestion decrease factor
    double m_gamma; // Remote congestion decrease factor
    double m_addRttSuppress;
    bool m_reactToCongestionMarks;  


    // CUBIC
    static constexpr double m_cubic_c = 0.4;
    static constexpr double m_cubicBeta = 0.7;
    bool m_useCubicFastConv;
    std::map<std::string, double> m_cubicWmax;
    std::map<std::string, double> m_cubicLastWmax;

    // Interest sending rate pacing
    std::map<std::string, EventId> m_scheduleEvent;
    std::map<std::string, EventId> m_sendEvent;
    bool isRTTEstimated;
    int m_initPace;

    //! QueueSize-based CC
    int m_queueThreshold;
    int m_inflightThreshold;
    double m_qsMDFactor;
    double m_qsRPFactor;
    int m_qsTimeDuration;
    double m_qsInitRate; // Unit: pgks/ms
    std::map<std::string, bool> firstData;
    std::map<std::string, utils::SlidingWindow<double>> m_qsSlidingWindows;
    std::map<std::string, EventId> m_rateEvent;    
    std::map<std::string, double> m_rateLimit; // Unit: pkgs/ms
    std::map<std::string, double> m_estimatedBW; // Unit: pkgs/ms
    std::map<std::string, int64_t> RTT_estimation_qs; // Unit: ms
    std::map<std::string, bool> nackSignal; // NACK signal for each flow
    std::map<std::string, bool> timeoutSignal; // Timeout signal for each flow
    std::map<std::string, double> m_lastBW;    
    std::map<std::string, std::string> m_ccState;
    std::map<std::string, double> m_inflightLimit; // Inflight limit for each flow
    

    // Global flow map
    std::vector<std::vector<std::string>> globalTreeRound; // First dimension: round. Second dimension: next-tier leaves, initialized in ConstructAggregationTree()
    int linkCount;

    std::map<std::string, int> RTT_count; // How many RTT packets this node has received, used to estimate how many iterations have passed

    //? The following is new design for windowed average RTT
    std::map<std::string, std::deque<int64_t>> RTT_windowed_queue; // Store windowed average RTT for each flow
    std::map<std::string, int64_t> RTT_historical_estimation; // Historical RTT estimation    
    int m_smooth_window_size; // Window size for RTT windowed average

    //* initSeq is only used once, when broadcasting the aggregation tree
    uint32_t initSeq;

    // Track seq of each flow when sending interests
    //? Track the global iteration when interest splitting
    uint32_t globalSeq;
    //? Track the latest iteration has been sent. Function - determine whether it's the correct time to start aggregation time
    std::map<std::string, uint32_t> SeqMap; // Key: flow, Value: seq
    //? Keep a interest queue for each flow
    std::map<std::string, std::deque<uint32_t>> interestQueue; // Key: flow, Value: queue



    // Get producer list, which are used to generate new interests
    std::string proList;

    // Constructed aggregation Tree
    std::vector<std::map<std::string, std::vector<std::string>>> aggregationTree; // Entire tree (including main tree and sub-trees)

    // Broadcast aggregation tree
    bool broadcastSync;
    std::set<std::string> broadcastList; // Elements within the set need to be broadcasted, all elements are unique

    // Aggregation synchronization
    std::map<uint32_t, std::vector<std::string>> map_agg_oldSeq_newName; // Manage names for entire iteration
    std::map<uint32_t, bool> m_agg_finished; // Manage whether aggregation is finished for each iteration

    // Used inside InterestGenerator
    std::map<std::string, std::string> NameSec0_2; // Key: flow, Value: first three segments of interest name, e.g. "/agg0/pro0.pro1/data"    
    std::vector<std::string> vec_iteration; // Store upstream nodes' name


    // Timeout check/ RTO measurement
    std::map<std::string, ns3::Time> m_timeoutCheck;
    std::map<std::string, Time> RTO_threshold;
    std::map<std::string, int64_t> SRTT;
    std::map<std::string, int64_t> RTTVAR;
    std::map<std::string, bool> initRTO;
    // TODO: Count the number of timeout, testing purpose only
    std::map<std::string, int> numTimeout;


    // Designed for actual aggregation operations
    std::map<uint32_t, bool> partialAggResult;
    std::map<uint32_t, std::vector<double>> sumParameters;
    std::map<uint32_t, std::vector<double>> aggregationResult;
    int producerCount;

    // Congestion signal
    bool ECNLocal;
    bool ECNRemote;

    // defined for response time
    std::map<std::string, ns3::Time> rttStartTime;
    std::map<std::string, ns3::Time> responseTime;
    int64_t total_response_time;
    int round;

    // defined for aggregation time
    std::map<uint32_t, ns3::Time> aggregateStartTime;
    std::map<uint32_t, ns3::Time> aggregateTime;
    int64_t totalAggregateTime;
    int iterationCount;

    // New defined attribute variables
    std::string m_topologyType;
    std::string m_interestName; // Consumer's interest prefix
    std::string m_nodeprefix; // Consumer's node prefix
    uint32_t m_iteNum; // The number of iterations
    int m_interestQueue; // Interest queue size
    int m_dataQueue; // Data queue size
    int m_dataSize; // Data size
    int m_constraint; // Constraint of each sub-tree
    double m_EWMAFactor; // Factor used in EWMA, recommended value is between 0.1 and 0.3
    double m_thresholdFactor; // Factor to compute "RTT_threshold", i.e. "RTT_threshold = Threshold_factor * RTT_measurement"
    bool m_useWIS; // Whether to suppress the window increasing rate after congestion

    Name m_prefix;
    Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
    uint32_t m_seq;      ///< @brief currently requested sequence number
    uint32_t m_seqMax;   ///< @brief maximum number of sequence number
    Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
    EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

    Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

    Time m_offTime;          ///< \brief Time interval between packets
    Time m_interestLifeTime; ///< \brief LifeTime for interest packet



    /// @cond include_hidden
      /**
    * \struct This struct contains sequence numbers of packets to be retransmitted
    */
    struct RetxSeqsContainer : public std::set<uint32_t> {
    };

    RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
  /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  /// @endcond
};

} // namespace ndn
} // namespace ns3

#endif // NDN_CONSUMER_H
