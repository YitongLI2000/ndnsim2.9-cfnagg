//
// Created by Yitong LI on 16/4/2024.
//

#ifndef NDN_AGGREGATOR_HPP
#define NDN_AGGREGATOR_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "sliding-window.hpp"
#include "ndn-app.hpp"
#include "ModelData.hpp"
#include "ndn-cxx/lp/nack.hpp"
#include "ndn-cxx/lp/nack-header.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-value.h"
#include "ns3/log.h"
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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/circular_buffer.hpp>

namespace ns3{
namespace ndn{




class Aggregator : public App
{
public:


    /**
     * Initiate attributes for consumer class, some of them may be used, some are optional
     * @return Total TypeId
     */
    static TypeId GetTypeId();


    /**
     * Constructor
     */
    Aggregator();
    virtual ~Aggregator(){};


    // Core

    /**
     * Process incoming interest packets
     * @param interest
     */
    virtual void 
    OnInterest(shared_ptr<const Interest> interest);


    /**
     * Process incoming data packets
     * @param data
     */
    virtual void 
    OnData(shared_ptr<const Data> data);


    /**
     * Process incoming data packets
     * @param prefix
     */
    virtual void 
    ScheduleNextPacket(std::string prefix);


    /**
     * Divide interest into several new interests, performed whe receiving aggregation tree
     */
    bool 
    InterestSplitting(uint32_t seq);


    /**
     * Generate interests for all iterations and push them into queue for further interest sending scheduling
     * First invoke when start simulation, every time when one iteration finished aggregation, update interestQueue again
     * The number of interests of each round are checked, if any of them is not enough, generate interests into the queue
     */
    void 
    InterestGenerator();


    /**
     * Check whether interest buffer is empty, if not, send new interests
     */
    void 
    SendPacket(std::string prefix);


    /**
     * Format the interest packet and send out interests
     * @param newName
     */
    void 
    SendInterest(shared_ptr<Name> newName);


    /**
     * Send data packets
     * @param data
     */
    void 
    SendData(uint32_t seq);


    /**
     * Send Nack back in the format of data packet
     * Currently used when interest queue is overflowed
     * @param interestName
     */
    void 
    SendNack(shared_ptr<const Interest> interest);


    /**
     * Triggered when timeout
     * @param nameString
     */
    virtual void 
    OnTimeout(std::string nameString);


    /**
     * Invoked when Nack
     * @param nack
     */
    virtual void 
    OnNack(shared_ptr<const lp::Nack> nack);


    /**
     * Set cwnd
     * @param window
     */
    void 
    SetWindow(uint32_t payload);


    /**
     * Get cwnd
     * @return cwnd
     */
    uint32_t 
    GetWindow() const;


    /**
     * Set max sequence number, not used now
     * @param seqMax
     */
    void 
    SetSeqMax(uint32_t seqMax);


    /**
     * Get max sequence number, not used now
     * @return
     */
    uint32_t 
    GetSeqMax() const;


    /**
     * Increase cwnd
     * @param prefix Flow name
     */
    void 
    WindowIncrease(std::string prefix);


    /**
     * Decrease cwnd
     * @param prefix Flow name
     * @param type Congestion type
     */
    void 
    WindowDecrease(std::string prefix, std::string type);


    /**
     * Cubic increase
     * @param prefix Flow name
     */
    void 
    CubicIncerase(std::string prefix);


    /**
     * Cubic decrease
     * @param prefix Flow name
     * @param type Congestion type
     */
    void 
    CubicDecrease(std::string prefix, std::string type);


    /**
     * Perform aggregation for incoming data packets (sum)
     * @param data
     * @param dataName
     */
    void 
    Aggregate(const ModelData& data, const uint32_t& seq);


    /**
     * Don't get mean for now, just reformating the data packets, perform aggregation at consumer
     * @param dataName
     * @return Data content
     */
    ModelData 
    GetMean(const uint32_t& seq);

    //! Updated QueueSize-based CC

    /**
     * Based on returned data, update rtt estimation
     * @param prefix
     * @param resTime unit - us
     */
    void
    RTTMeasure(std::string prefix, int64_t resTime);


    /**
     * Get the data rate and return with correct value from the sliding window
     * @param prefix flow name
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
     */
    void
    RateLimitUpdate(std::string prefix);



    /**
     * Sum response time
     * @param response_time
     */
    void 
    ResponseTimeSum (int64_t response_time);


    /**
     * Compute average for response time
     * @return Average response time
     */
    int64_t 
    GetResponseTimeAverage();


    /**
     * Sum aggregation time from each iteration
     * @param aggregate_time
     */
    void 
    AggregateTimeSum (int64_t response_time);


    /**
     * Get average aggregation time
     * @return Average aggregation time
     */
    int64_t 
    GetAggregateTimeAverage();


    /**
     * Measure new RTO
     * @param prefix
     * @param resTime unit - us
     * @return New RTO
     */
    void 
    RTOMeasure(std::string prefix, int64_t resTime);

    /**
     * Based on RTT of the first iteration, compute their RTT average as threshold, use the threshold for congestion control
     * Apply Exponentially Weighted Moving Average (EWMA) for RTT Threshold Computation
     * @param prefix
     * @param responseTime
     * @return congestion signal
     */
    bool 
    CongestionDetection(std::string prefix, int64_t responseTime);


    /**
     * Intermediate function to parse string, used in aggTreeProcessStrings()
     * Changed from set into list
     * @param input
     * @return Parsed string
     */
    std::pair<std::string, std::vector<std::string>> 
    aggTreeProcessSingleString(const std::string& input);


    /**
     * When receive "initialization" message (tree construction) from consumer, parse the message to get required info
     * Changed from set into list
     * @param inputs
     * @return A map consist child node info for current aggregator
     */    
    std::map<std::string, std::vector<std::string>> 
    aggTreeProcessStrings(const std::vector<std::string>& inputs);

    // Utility function
    /**
     * Get data queue of certain flow
     */
    double 
    getDataQueueSize(std::string prefix);


    // Logging function

    /**
     * Record window when receiving a new packet
     */
    void 
    WindowRecorder(std::string prefix);


    /**
     * Record in-flight packets when receiving a new packet
     */
    void 
    InFlightRecorder(std::string prefix);


    /**
     * Record the response time for each returned packet, store them in a file
     * @param responseTime
     */
    void 
    ResponseTimeRecorder(Time responseTime, uint32_t seq, std::string prefix);


    /**
     * Record RTO when receiving data packet
     * @param prefix
     */
    void 
    RTORecorder(std::string prefix);


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
     * Initialize all parameters
     * Initialize timeout checking mechanism, remove part of SetRetcTimer()'s function to here
     */
    void 
    InitializeParameters();


    /**
     * Check whether the cwnd has been decreased within the last RTT duration
     * @param threshold
     */
    bool 
    CanDecreaseWindow(std::string prefix, int64_t threshold);


    /**
     * Record the final throughput into file at the end of simulation
     * @param interestThroughput
     * @param dataThroughput
     */
    void 
    ThroughputRecorder(int interestThroughput, int dataThroughput, Time start_simulation);


    /**
     * Record the final results
     */
    void 
    ResultRecorder(int64_t aveAggTime);


    /**
     * Record queue size based CC info
     */
    void
    QueueRecorder(std::string prefix, double queueSize);

protected:

    /**
     * Override, start aggregtator application
     */
    virtual void 
    StartApplication() override;


    /**
     * Override, stop application
     */
    virtual void 
    StopApplication() override;


    /**
     * Check timeout every certain interval
     */
    void 
    CheckRetxTimeout();


    /**
     * Set initial timeout checking interval
     * @param retxTimer
     */
    void 
    SetRetxTimer(Time retxTimer);


    /**
     * Get timeout checking interval
     * @return Timeout checking interval
     */
    Time 
    GetRetxTimer() const;



protected:
    Ptr<UniformRandomVariable> m_rand;
    Name m_prefix;
    Name m_nexthop;
    Name m_nexttype;
    Time m_interestLifeTime;


public:
    typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
    typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

protected:
    // log file
    std::string folderPath = "src/ndnSIM/results/logs/agg";

    // All logs start to write after synchronization, make sure only the chosen aggregators will generate log files; those aren't chosen will disable this function
    std::map<std::string, std::string> RTO_recorder;
    std::map<std::string, std::string> window_recorder;
    std::map<std::string, std::string> responseTime_recorder;
    std::map<std::string, std::string> inFlight_recorder;
    std::map<std::string, std::string> qsNew_recorder; //! Updated queue size CC
    std::string aggTable_recorder;
    std::string aggregateTime_recorder;

    int suspiciousPacketCount; // Record the number of timeout
    int downstreamRetxCount; // Record the number of retransmission interests from downstream
    int interestOverflow; // Record the number of interest overflow within interest queue
    int dataOverflow; // Record the number of data overflow within data queue
    int nackCount; // Record the number of NACK

    // Local throughput measurement
    int totalInterestThroughput;
    int totalDataThroughput;
    Time startSimulation;
    Time stopSimulation;

    // Tree broadcast synchronization
    bool treeSync;

    // Congestion control, measure RTT threshold to detect congestion
    int numChild; // Start congestion control after 3 iterations
    std::map<std::string, int> RTT_count; // How many RTT packets this node has received, used to estimate how many iterations have passed
    
    //? The following is new design for windowed average RTT
    std::map<std::string, std::deque<int64_t>> RTT_windowed_queue; // Store windowed average RTT for each flow
    std::map<std::string, int64_t> RTT_historical_estimation; // Historical RTT estimation    
    int m_smooth_window_size; // Window size for RTT windowed average

    // Congestion signal
    bool ECNLocal;
    bool ECNRemote;

    // Basic cwnd management
    uint32_t m_initialWindow;
    std::map<std::string, double> m_window;
    uint32_t m_minWindow;
    std::map<std::string, uint32_t> m_inFlight;
    std::map<std::string, double> m_ssthresh;
    bool m_setInitialWindowOnTimeout;

    // Window decrease suppression
    std::map<std::string, Time> lastWindowDecreaseTime;
    bool isWindowDecreaseSuppressed;

    // CUBIC
    static constexpr double m_cubic_c = 0.4;
    static constexpr double m_cubicBeta = 0.7;
    bool m_useCubicFastConv;
    std::map<std::string, double> m_cubicWmax;
    std::map<std::string, double> m_cubicLastWmax;


    // AIMD
    std::map<std::string, uint32_t> lastCongestionSeq;
    std::map<std::string, int> successiveCongestion;
    bool m_useCwa;
    double m_alpha; // Timeout decrease factor
    double m_beta; // Local congestion decrease factor
    double m_gamma; // Remote congestion decrease factor
    double m_EWMAFactor; // Factor used in EWMA, recommended value is between 0.1 and 0.3
    double m_thresholdFactor; // Factor to compute "RTT_threshold", i.e. "RTT_threshold = Threshold_factor * RTT_measurement"
    bool m_useWIS; // Whether to suppress the window increasing rate after congestion
    bool m_reactToCongestionMarks; // PCON's implementation, disable it for now
    int m_interestQueue; // Max interest queue size
    int m_dataQueue; // Max data queue size
    int m_dataSize; // Max data size
    uint32_t m_iteNum;

    // TODO:debugging this section now
    // Interest sending rate pacing
    std::map<std::string, EventId> m_scheduleEvent;
    std::map<std::string, EventId> m_sendEvent;
    bool firstInterest;
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



    // Interest queue (for each flow), keep tracking about each flow
    std::map<std::string, std::deque<uint32_t>> interestQueue;

    // Interest splitting - divided interests
    std::map<std::string, std::string> NameSec0_2;
    std::map<std::string, uint32_t> SeqMap;
    std::vector<std::string> vec_iteration; // Store upstream nodes' name

    // Timeout check and RTT measurement
    std::map<std::string, Time> m_timeoutCheck;
    std::map<std::string, int64_t> SRTT;
    std::map<std::string, int64_t> RTTVAR;
    std::map<std::string, int> roundRTT;
    std::map<std::string, Time> RTO_threshold;
    std::map<std::string, int> numTimeout;

    // Aggregation list
    std::map<uint32_t, std::string> m_agg_newDataName; // whole name
    std::map<uint32_t, std::vector<std::string>> map_agg_oldSeq_newName; // name segments



    // Aggregation variables storage
    std::map<uint32_t, bool> partialAggResult;
    std::map<uint32_t, std::vector<double>> sumParameters; // result after performing aggregation(arithmetic add them together)
    std::map<uint32_t, std::vector<std::string>> congestionSignalList; // result after aggregating congestion signal
    std::map<uint32_t, bool> congestionSignal; // congestion signal for current node

    // Response/Aggregation time measurement
    std::map<std::string, Time> rttStartTime;
    std::map<std::string, Time> responseTime;
    int64_t totalResponseTime;
    int round;

    std::map<uint32_t, Time> aggregateStartTime;
    std::map<uint32_t, Time> aggregateTime;
    int64_t totalAggregateTime;
    int iterationCount;

    // Receive aggregation tree from consumer
    std::map<std::string, std::vector<std::string>> aggregationMap;


    uint32_t m_seq;      ///< @brief currently requested sequence number
    uint32_t m_seqMax;   ///< @brief maximum number of sequence number
    Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
    EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed
    Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator
    Time m_offTime;          ///< \brief Time interval between packets
    Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
    Time m_freshness;
    uint32_t m_signature;
    Name m_keyLocator;

    struct RetxSeqsContainer : public std::set<uint32_t> {
    };

    RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted


    struct SeqTimeout {
        SeqTimeout(uint32_t _seq, Time _time)
            : seq(_seq)
            , time(_time)
        {
        }

        uint32_t seq;
        Time time;
        };

class i_seq {};

class i_timestamp {};


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


TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/> m_lastRetransmittedInterestDataDelay;
TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

};

} // namespace ndn
} // namespace ns3


#endif //APPS_NDN_AGGREGATOR_HPP
