/**
 * This class is written by Yitong, serves as aggregator in CFNAgg
 * This class implements all functions for CFNAgg, which is slightly different from consumer
 * (aggregators serve as intermediate nodes, while consumer is the end node to generate new interests)
 *
 */

#include "ndn-aggregator.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "ModelData.hpp"
#include "ndn-consumer.hpp"

#include "ns3/log.h"
#include "ns3/codel-queue-disc.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include <limits>
#include <cmath>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <iostream>
#include <sstream>

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.Aggregator");

namespace ns3{
namespace ndn{

NS_OBJECT_ENSURE_REGISTERED(Aggregator);



TypeId
Aggregator::GetTypeId(void)
{
    static TypeId tid =
            TypeId("ns3::ndn::Aggregator")
            .SetGroupName("Ndn")
            .SetParent<App>()
            .AddConstructor<Aggregator>()
            .AddAttribute("StartSeq",
                        "Starting sequence number",
                        IntegerValue(0),
                        MakeIntegerAccessor(&Aggregator::m_seq),
                        MakeIntegerChecker<int32_t>())
            .AddAttribute("Prefix",
                        "Interest prefix/name",
                        StringValue("/"),
                        MakeNameAccessor(&Aggregator::m_prefix),
                        MakeNameChecker())
            .AddAttribute("LifeTime",
                        "Life time for interest packet",
                        StringValue("4s"),
                        MakeTimeAccessor(&Aggregator::m_interestLifeTime),
                        MakeTimeChecker())
            .AddAttribute("RTTWindowSize",
                        "The smooth windowed average size for RTT",
                        IntegerValue(3),
                        MakeIntegerAccessor(&Aggregator::m_smooth_window_size),
                        MakeIntegerChecker<int>())               
            .AddAttribute("RetxTimer",
                        "Timeout defining how frequent retransmission timeouts should be checked",
                        StringValue("20ms"),
                        MakeTimeAccessor(&Aggregator::GetRetxTimer, &Aggregator::SetRetxTimer),
                        MakeTimeChecker())
            .AddAttribute("Freshness",
                        "Freshness of data packets, if 0, then unlimited freshness",
                        TimeValue(Seconds(0)),
                        MakeTimeAccessor(&Aggregator::m_freshness),
                        MakeTimeChecker())
            .AddAttribute("Signature",
                        "Fake signature, 0 valid signature (default), other values application-specific",
                        UintegerValue(0),
                        MakeUintegerAccessor(&Aggregator::m_signature),
                        MakeUintegerChecker<uint32_t>())
            .AddAttribute("KeyLocator",
                        "Name to be used for key locator.  If root, then key locator is not used",
                        NameValue(),
                        MakeNameAccessor(&Aggregator::m_keyLocator),
                        MakeNameChecker())
            .AddAttribute("Window",
                        "Initial size of the window",
                        StringValue("1"),
                        MakeUintegerAccessor(&Aggregator::GetWindow, &Aggregator::SetWindow),
                        MakeUintegerChecker<uint32_t>())
            .AddAttribute("InitPace",
                        "Initial size of the interest sending pace, default is 2 ms",
                        IntegerValue(2),
                        MakeIntegerAccessor(&Aggregator::m_initPace),
                        MakeIntegerChecker<int>())                          
            .AddAttribute("MaxSeq",
                        "Maximum sequence number to request (alternative to Size attribute, would activate only if Size is -1). The parameter is activated only if Size negative (not set)",
                        IntegerValue(std::numeric_limits<uint32_t>::max()),
                        MakeUintegerAccessor(&Aggregator::GetSeqMax, &Aggregator::SetSeqMax),
                        MakeUintegerChecker<uint32_t>())
            .AddAttribute("InitialWindowOnTimeout",
                        "Set window to initial value when timeout occurs",
                        BooleanValue(true),
                        MakeBooleanAccessor(&Aggregator::m_setInitialWindowOnTimeout),
                        MakeBooleanChecker())
            .AddAttribute("Alpha",
                        "TCP Multiplicative Decrease factor",
                        DoubleValue(0.5),
                        MakeDoubleAccessor(&Aggregator::m_alpha),
                        MakeDoubleChecker<double>())
            .AddAttribute("Beta",
                        "Local congestion decrease factor",
                        DoubleValue(0.6),
                        MakeDoubleAccessor(&Aggregator::m_beta),
                        MakeDoubleChecker<double>())
            .AddAttribute("Gamma",
                        "Remote congestion decrease factor",
                        DoubleValue(0.7),
                        MakeDoubleAccessor(&Aggregator::m_gamma),
                        MakeDoubleChecker<double>())
            .AddAttribute("EWMAFactor",
                        "EWMA factor used when measuring RTT, recommended between 0.1 and 0.3",
                        DoubleValue(0.3),
                        MakeDoubleAccessor(&Aggregator::m_EWMAFactor),
                        MakeDoubleChecker<double>())
            .AddAttribute("ThresholdFactor",
                        "Factor to compute actual RTT threshold",
                        DoubleValue(1.0),
                        MakeDoubleAccessor(&Aggregator::m_thresholdFactor),
                        MakeDoubleChecker<double>())
            .AddAttribute("CcAlgorithm",
                        "Specify which window adaptation algorithm to use (AIMD, or CUBIC)",
                        EnumValue(CcAlgorithm::AIMD),
                        MakeEnumAccessor(&Aggregator::m_ccAlgorithm),
                        MakeEnumChecker(CcAlgorithm::AIMD, "AIMD", CcAlgorithm::CUBIC, "CUBIC"))                          
            .AddAttribute("Iteration",
                        "The number of iterations to run in the simulation",
                        IntegerValue(200),
                        MakeIntegerAccessor(&Aggregator::m_iteNum),
                        MakeIntegerChecker<int32_t>())
            .AddAttribute("UseWIS",
                        "Suppress the window increasing rate after congestion",
                        BooleanValue(true),
                        MakeBooleanAccessor(&Aggregator::m_useWIS),
                        MakeBooleanChecker())
            .AddAttribute("ReactToCongestionMarks",
                        "If true, process received congestion marks",
                        BooleanValue(true),
                        MakeBooleanAccessor(&Aggregator::m_reactToCongestionMarks),
                        MakeBooleanChecker())
            .AddAttribute("UseCwa",
                        "If true, use Conservative Window Adaptation",
                        BooleanValue(false),
                        MakeBooleanAccessor(&Aggregator::m_useCwa),
                        MakeBooleanChecker())
            .AddAttribute("UseCubicFastConv",
                        "If true, use Fast Convergence for Cubic",
                        BooleanValue(false),
                        MakeBooleanAccessor(&Aggregator::m_useCubicFastConv),
                        MakeBooleanChecker())
            .AddAttribute("InterestQueueSize",
                        "Define the interest queue size",
                        IntegerValue(10),
                        MakeIntegerAccessor(&Aggregator::m_interestQueue),
                        MakeIntegerChecker<int>())
            .AddAttribute("DataQueueSize",
                        "Define the data queue size",
                        IntegerValue(10),
                        MakeIntegerAccessor(&Aggregator::m_dataQueue),
                        MakeIntegerChecker<int>())
            .AddAttribute("DataSize",
                        "Define the data content size",
                        IntegerValue(150),
                        MakeIntegerAccessor(&Aggregator::m_dataSize),
                        MakeIntegerChecker<int>())
            .AddAttribute("AggQueueThreshold",
                        "Data queue threshold",
                        IntegerValue(10),
                        MakeIntegerAccessor(&Aggregator::m_queueThreshold),
                        MakeIntegerChecker<int>())
            .AddAttribute("InFlightThreshold",
                        "Inflight threshold",
                        IntegerValue(20),
                        MakeIntegerAccessor(&Aggregator::m_inflightThreshold),
                        MakeIntegerChecker<int>())                        
            .AddAttribute("QSMDFactor",
                        "QueueSize-based CC's multiplicative decrease factor",
                        DoubleValue(0.9),
                        MakeDoubleAccessor(&Aggregator::m_qsMDFactor),
                        MakeDoubleChecker<double>())
            .AddAttribute("QSRPFactor",
                        "QueueSize-based CC's rate probing factor",
                        DoubleValue(1.05),
                        MakeDoubleAccessor(&Aggregator::m_qsRPFactor),
                        MakeDoubleChecker<double>())
            .AddAttribute("QSSlidingWindowDuration",
                        "QueueSize-based CC's sliding window's time duration - Unit: ms",
                        IntegerValue(10),
                        MakeIntegerAccessor(&Aggregator::m_qsTimeDuration),
                        MakeIntegerChecker<int>())                        
            .AddAttribute("QSInitRate",
                        "QueueSize-based CC's initial interest sending rate, default set as 0.002 pkgs/us",
                        DoubleValue(0.002),
                        MakeDoubleAccessor(&Aggregator::m_qsInitRate),
                        MakeDoubleChecker<double>());
     
    return tid;
}



Aggregator::Aggregator()
    : suspiciousPacketCount(0)
    , downstreamRetxCount(0)
    , interestOverflow(0)
    , dataOverflow(0)
    , nackCount(0)
    , totalInterestThroughput(0)
    , totalDataThroughput(0)
    , m_rand(CreateObject<UniformRandomVariable>())
    , treeSync(false)
    , m_seq(0)
    , m_minWindow(1)
    , totalResponseTime(0)
    , round(0)
    , totalAggregateTime(0)
    , iterationCount(0)
{
    m_rtt = CreateObject<RttMeanDeviation>();
}



std::pair<std::string, std::vector<std::string>>
Aggregator::aggTreeProcessSingleString(const std::string& input)
{
    std::istringstream iss(input);
    std::string segment;
    std::vector<std::string> segments;

    // Use getline to split the string by '.'
    while (getline(iss, segment, '.')) {
        segments.push_back(segment);
    }

    // Check if there are enough segments to form a key and a vector
    if (segments.size() > 1) {
        std::string key = segments[0];
        std::vector<std::string> values(segments.begin() + 1, segments.end());
        return {key, values};
    }

    return {};  // Return an empty pair if not enough segments
}



std::map<std::string, std::vector<std::string>>
Aggregator::aggTreeProcessStrings(const std::vector<std::string>& inputs)
{
    std::map<std::string, std::vector<std::string>> result;

    for (const std::string& input : inputs) {
        auto entry = aggTreeProcessSingleString(input);
        if (!entry.first.empty()) {
            // Append all elements from entry.second to the vector at result[entry.first]
            result[entry.first].insert(
                result[entry.first].end(),
                entry.second.begin(),
                entry.second.end()
            );
        }
    }

    return result;
}



double 
Aggregator::getDataQueueSize(std::string prefix)
{
    double queueSize = 0.0;
    for (const auto& [seq, aggList] : map_agg_oldSeq_newName) {
        if (std::find(aggList.begin(), aggList.end(), prefix) == aggList.end()) {
            queueSize += 1.0;
        }
    }

    NS_LOG_DEBUG("Flow: " << prefix << " -> Data queue size: " << queueSize);
    return queueSize;
}



void
Aggregator::ResponseTimeSum (int64_t response_time)
{
    totalResponseTime += response_time;
    ++round;
}



int64_t
Aggregator::GetResponseTimeAverage()
{
    if (round == 0)
    {
        NS_LOG_DEBUG("Error happened when calculating average response time!");
        return 0;
    }

    return totalResponseTime / round;
}



void
Aggregator::AggregateTimeSum (int64_t aggregate_time)
{
    totalAggregateTime += aggregate_time;
    ++iterationCount;
}



int64_t
Aggregator::GetAggregateTimeAverage()
{
    if (iterationCount == 0)
    {
        NS_LOG_DEBUG("Error happened when calculating aggregate time!");
        return 0;
    }

    return totalAggregateTime / iterationCount;
}



void
Aggregator::CheckRetxTimeout()
{
    Time now = Simulator::Now();

    for (auto it = m_timeoutCheck.begin(); it != m_timeoutCheck.end();){
        std::string name = it->first;
        std::string flow = make_shared<Name>(name)->get(0).toUri();
        if (now - it->second > RTO_threshold[flow]) {
            it = m_timeoutCheck.erase(it);
            OnTimeout(name);
        } else {
            ++it;
        }
    }
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Aggregator::CheckRetxTimeout, this);
}



bool
Aggregator::CongestionDetection(std::string prefix, int64_t responseTime)
{
    //* Normal usage is "push_back" "pop_front"
    // Update RTT windowed queue and historical estimation
    RTT_windowed_queue[prefix].push_back(responseTime);
    RTT_count[prefix]++;

    if (RTT_windowed_queue[prefix].size() > m_smooth_window_size) {
        int64_t transitionValue = RTT_windowed_queue[prefix].front();
        RTT_windowed_queue[prefix].pop_front();

        if (RTT_historical_estimation[prefix] == 0) {
            RTT_historical_estimation[prefix] = transitionValue;
        } else {
            RTT_historical_estimation[prefix] = m_EWMAFactor * transitionValue + (1 - m_EWMAFactor) * RTT_historical_estimation[prefix];
        }
    }
    else {
        NS_LOG_DEBUG("RTT_windowed_queue size: " << RTT_windowed_queue[prefix].size());
    }

    // Detect congestion
    if (RTT_count[prefix] >= 2 * m_smooth_window_size) {
        int64_t pastRTTAverage = 0;
        for (int64_t pastRTT : RTT_windowed_queue[prefix]) {
            pastRTTAverage += pastRTT;
        }
        pastRTTAverage /= m_smooth_window_size;
        
        // Enable RTT-estimation for scheduler
        isRTTEstimated = true;

        int64_t rtt_threshold = m_thresholdFactor * RTT_historical_estimation[prefix];
        if (rtt_threshold < pastRTTAverage) {
            return true;
        } else {
            return false;
        }
    }
    else {
        NS_LOG_DEBUG("RTT_count: " << RTT_count[prefix]);
        return false;
    }
}



void
Aggregator::RTOMeasure(std::string prefix, int64_t resTime)
{
    if (roundRTT[prefix] == 0) {
        RTTVAR[prefix] = resTime / 2;
        SRTT[prefix] = resTime;
    } else {
        RTTVAR[prefix] = 0.75 * RTTVAR[prefix] + 0.25 * std::abs(SRTT[prefix] - resTime); // RTTVAR = (1 - b) * RTTVAR + b * |SRTT - RTTsample|, where b = 0.25
        SRTT[prefix] = 0.875 * SRTT[prefix] + 0.125 * resTime; // SRTT = (1 - a) * SRTT + a * RTTsample, where a = 0.125
    }
    roundRTT[prefix]++;
    int64_t RTO = SRTT[prefix] + 4 * RTTVAR[prefix]; // RTO = SRTT + K * RTTVAR, where K = 4

    //RTO_threshold[prefix] = MicroSeconds(4 * RTO);
    RTO_threshold[prefix] = MicroSeconds(2 * RTO);
}



void
Aggregator::OnTimeout(std::string nameString)
{
    shared_ptr<Name> name = make_shared<Name>(nameString);
    std::string name_sec0 = name->get(0).toUri();
    uint32_t seq = name->get(-1).toSequenceNumber();
    NS_LOG_DEBUG("Flow " << name_sec0 << " - name -> " << nameString <<": timeout.");

    if (m_inFlight[name_sec0] > 0) {
        m_inFlight[name_sec0]--;
    } else {
        NS_LOG_DEBUG("Error when timeout, please exit and check!");
        Simulator::Stop();
        return;
    }

    // Handle timeout on next CC round
    // QueueSize-based CC
    timeoutSignal[name_sec0] = true;
    
    if (interestQueue.find(name_sec0) == interestQueue.end()) {
        NS_LOG_DEBUG("Error when timeout, please exit and check!");
        Simulator::Stop();
        return;
    }
    interestQueue[name_sec0].push_front(seq);

    suspiciousPacketCount++;
}



void
Aggregator::SetRetxTimer(Time retxTimer)
{
    m_retxTimer = retxTimer;
    if (m_retxEvent.IsRunning()) {
        Simulator::Remove(m_retxEvent);
    }

    m_retxEvent = Simulator::Schedule(m_retxTimer, &Aggregator::CheckRetxTimeout, this);
}



Time
Aggregator::GetRetxTimer() const
{
    return m_retxTimer;
}



void
Aggregator::StartApplication()
{
    //NS_LOG_FUNCTION_NOARGS();
    App::StartApplication();
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}



void
Aggregator::StopApplication()
{
    // Cancel packet generation - can be a way to stop simulation gracefully?
    //Simulator::Cancel(m_sendEvent);
    App::StopApplication();
}



void Aggregator::Aggregate(const ModelData& data, const uint32_t& seq) {
    // first initialization
    if (sumParameters.find(seq) == sumParameters.end()){
        sumParameters[seq] = std::vector<double>(m_dataSize, 0.0f);
    }

    // Aggregate data
    std::transform(sumParameters[seq].begin(), sumParameters[seq].end(), data.parameters.begin(), sumParameters[seq].begin(), std::plus<double>());

    // Aggregate congestion signal
    congestionSignalList[seq].insert(congestionSignalList[seq].end(), data.congestedNodes.begin(), data.congestedNodes.end());
}



ModelData 
Aggregator::GetMean(const uint32_t& seq)
{
    ModelData result;
    if (sumParameters.find(seq) != sumParameters.end()) {
        result.parameters = sumParameters[seq];

/*         // Encapsulate qsf as meta data
        double maxQsf = 0;
        for (const auto& [key, value] : aggregationMap) {
            maxQsf = std::max(maxQsf, static_cast<double>(interestQueue[key].size()));
        }

        maxQsf = std::max(maxQsf, static_cast<double>(sumParameters.size()));

        NS_LOG_INFO("Final QSF: " << maxQsf);
        result.qsf = maxQsf;

        // Add congestionSignal of current node if necessary, currently disable!
        if (congestionSignal[seq]) {
            congestionSignalList[seq].push_back(m_prefix.toUri());
            NS_LOG_DEBUG("Congestion detected on current node!");
        }
        result.congestedNodes = congestionSignalList[seq]; */
    } else {
        NS_LOG_DEBUG("Error when get aggregation result, please exit and check!");
        Simulator::Stop();
    }

    return result;
}



void
Aggregator::OnNack(shared_ptr<const lp::Nack> nack)
{
    App::OnNack(nack);
    NS_LOG_INFO("NACK received for: " << nack->getInterest().getName() << ", reason: " << nack->getReason());

    std::string dataName = nack->getInterest().getName().toUri();
    std::string name_sec0 = nack->getInterest().getName().get(0).toUri();
    uint32_t seq = nack->getInterest().getName().get(-1).toSequenceNumber();

    if (m_inFlight[name_sec0] > 0) {
        m_inFlight[name_sec0]--;
    } else {
        NS_LOG_DEBUG("InFlight number error, please exit and check!");
        Simulator::Stop();
        return;
    }

    // Handle nack
    interestQueue[name_sec0].push_front(seq);
    nackSignal[name_sec0] = true;


    // Stop tracing rtt and timeout
    rttStartTime.erase(dataName);
    m_timeoutCheck.erase(dataName);
    nackCount++;
}



void
Aggregator::SetWindow(uint32_t window)
{
    m_initialWindow = window;
}



uint32_t
Aggregator::GetWindow() const
{
    return m_initialWindow;
}



void
Aggregator::SetSeqMax(uint32_t seqMax)
{
    // Be careful, ignore maxSize here
    m_seqMax = seqMax;
}



uint32_t
Aggregator::GetSeqMax() const
{
    return m_seqMax;
}



void
Aggregator::WindowIncrease(std::string prefix)
{
    if (m_ccAlgorithm == CcAlgorithm::AIMD) {
        // If cwnd is larger than 8, check whether current bottleneck is because of downstream slow interest, if so, stop increasing cwnd
/*         if (m_window[prefix] > 8.0 && m_window[prefix] - m_inFlight[prefix] > 30.0){
            NS_LOG_DEBUG("Current bottleneck is downstream slow interest, stop increasing cwnd.");
        } else  */
        if (m_useWIS){
            if (m_window[prefix] < m_ssthresh[prefix]) {
                m_window[prefix] += 1.0;
            } else {
                m_window[prefix] += (1.0 / m_window[prefix]);
            }
            NS_LOG_DEBUG("Window size of flow '" << prefix << "' is increased to " << m_window[prefix]);        
        } else {
            m_window[prefix] += 1.0;
            NS_LOG_DEBUG("Window size of flow '" << prefix << "' is increased to " << m_window[prefix]);
        }        
    } else if (m_ccAlgorithm == CcAlgorithm::CUBIC) {
            CubicIncerase(prefix);
    } else {
        NS_LOG_DEBUG("CC alogrithm can't be recognized, please check!");
        Simulator::Stop();
    }
}



void
Aggregator::WindowDecrease(std::string prefix, std::string type)
{
    // Track last window decrease time
    lastWindowDecreaseTime[prefix] = Simulator::Now();

    // AIMD for timeout
    if (m_ccAlgorithm == CcAlgorithm::AIMD) {
        if (type == "timeout") {
            m_ssthresh[prefix] = m_window[prefix] * m_alpha;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else if (type == "nack") {
            m_ssthresh[prefix] = m_window[prefix] * m_alpha;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else if (type == "LocalCongestion") {
            m_ssthresh[prefix] = m_window[prefix] * m_beta;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else if (type == "RemoteCongestion") {
            m_ssthresh[prefix] = m_window[prefix] * m_gamma;
            m_window[prefix] = m_ssthresh[prefix];
        }      
    }
    else if (m_ccAlgorithm == CcAlgorithm::CUBIC) {
        if (type == "timeout") {
            m_ssthresh[prefix] = m_window[prefix] * m_alpha;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else if (type == "nack") {
            m_ssthresh[prefix] = m_window[prefix] * m_alpha;
            m_window[prefix] = m_ssthresh[prefix];
        }        
        else if (type == "LocalCongestion") {
            CubicDecrease(prefix, type);
        }
        else if (type == "RemoteCongestion") {
            // Do nothing, currently disabled
        }
    } else {
        NS_LOG_DEBUG("CC alogrithm can't be recognized, please check!");
        Simulator::Stop();
    }

    // Window size can't be reduced below 1
    if (m_window[prefix] < m_minWindow) {
        m_window[prefix] = m_minWindow;
    }
    NS_LOG_DEBUG("Window size of flow '" << prefix << "' is decreased to " << m_window[prefix] << ". Reason: " << type); 
}



void
Aggregator::CubicIncerase(std::string prefix) 
{
    // 1. Time since last congestion event in Seconds, round the value to 3 decimal places
    const double t = std::round(1000 * (Simulator::Now().GetMicroSeconds() - lastWindowDecreaseTime[prefix].GetMicroSeconds()) / 1e9) / 1000;
    NS_LOG_DEBUG("Time since last congestion event: " << t);

    // 2. Time it takes to increase the window to cubic_wmax
    // K = cubic_root(W_max*(1-beta_cubic)/C) (Eq. 2)
    const double k = std::cbrt(m_cubicWmax[prefix] * (1 - m_cubicBeta) / m_cubic_c);
    NS_LOG_DEBUG("K value: " << k);

    // 3. Target: W_cubic(t) = C*(t-K)^3 + W_max (Eq. 1)
    const double w_cubic = m_cubic_c * std::pow(t - k, 3) + m_cubicWmax[prefix];
    NS_LOG_DEBUG("Cubic increase target: " << w_cubic);

    // 4. Estimate of Reno Increase (Currently Disabled)
    //  const double rtt = m_rtt->GetCurrentEstimate().GetSeconds();
    //  const double w_est = m_cubic_wmax*m_beta + (3*(1-m_beta)/(1+m_beta)) * (t/rtt);

    //* TCP-friendly region, need to be disabled for ICN, "w_est" is not needed
    //constexpr double w_est = 0.0;

    //? Original cubic increase
/*     if (m_cubicWmax[prefix] <= 0) {
        NS_LOG_DEBUG("Error! Wmax is less than 0, check cubic increase!");
        Simulator::Stop();
    }

    double cubic_increment = std::max(w_cubic, 0.0) - m_window[prefix];
    // Cubic increment must be positive:
    // Note: This change is not part of the RFC, but I added it to improve performance.
    if (cubic_increment < 0) {
        cubic_increment = 0.0;
    }

    NS_LOG_DEBUG("Cubic increment: " << cubic_increment);
    m_window[prefix] += cubic_increment / m_window[prefix]; */

    //? Customized cubic increase
    if (m_window[prefix] < m_ssthresh[prefix]) {
        m_window[prefix] += 1.0;
    }
    else {
        if (m_cubicWmax[prefix] <= 0) {
            NS_LOG_DEBUG("Error! Wmax is less than 0, check cubic increase!");
            Simulator::Stop();
        }

        double cubic_increment = std::max(w_cubic, 0.0) - m_window[prefix];
        // Cubic increment must be positive:
        // Note: This change is not part of the RFC, but I added it to improve performance.
        if (cubic_increment < 0) {
            cubic_increment = 0.0;
        }

        NS_LOG_DEBUG("Cubic increment: " << cubic_increment);
        m_window[prefix] += cubic_increment / m_window[prefix];
    }

    NS_LOG_DEBUG("Window size of flow '" << prefix << "' is increased to " << m_window[prefix]);
}



void
Aggregator::CubicDecrease(std::string prefix, std::string type)
{
    //? Traditional cubic window decrease
    m_cubicWmax[prefix] = m_window[prefix];
    m_ssthresh[prefix] = m_window[prefix] * m_cubicBeta;
    m_ssthresh[prefix] = std::max<double>(m_ssthresh[prefix], m_minWindow);
    m_window[prefix] = m_window[prefix] * m_cubicBeta;

    //? Cubic with fast convergence
/*     const double FAST_CONV_DIFF = 1.0; // In percent
    // Current w_max < last_wmax
    if (m_useCubicFastConv && m_window < m_cubicLastWmax * (1 - FAST_CONV_DIFF / 100)) {
        m_cubicLastWmax = m_window;
        m_cubicWmax = m_window * (1.0 + m_cubicBeta) / 2.0;
    }
    else {
        // Save old cwnd as w_max:
        m_cubicLastWmax = m_window;
        m_cubicWmax = m_window;
    }

    m_ssthresh = m_window * m_cubicBeta;
    m_ssthresh = std::max<double>(m_ssthresh, m_initialWindow);
    m_window = m_ssthresh;

    m_cubicLastDecrease = time::steady_clock::now(); */
}



void
Aggregator::OnInterest(shared_ptr<const Interest> interest)
{
    NS_LOG_INFO("Receiving interest:  " << *interest);
    NS_LOG_DEBUG("The incoming interest packet size is: " << interest->wireEncode().size());
    App::OnInterest(interest);

    std::string interestType = interest->getName().get(-2).toUri();

    if (interestType == "data") {
        std::string originalName = interest->getName().toUri();
        uint32_t seq = interest->getName().get(-1).toSequenceNumber();
        bool isQueueFull = false;
        bool isDownstreamRetx = false;

        //? Check whether interest queue is full
        for (const auto& [key, value] : aggregationMap) {
            if (interestQueue[key].size() >= m_interestQueue) {
                isQueueFull = true;
                NS_LOG_DEBUG("Interest queue of flow " << key << " is full, drop it - " << interest->getName().toUri());
                interestOverflow++;
                
                // Interest queue overflow, send NACK back to downstream for notification
                SendNack(interest);
                return;
            }
        }

        //? Check whether the interest is retransmission from downstream
        if (m_agg_newDataName.find(seq) != m_agg_newDataName.end() || map_agg_oldSeq_newName.find(seq) != map_agg_oldSeq_newName.end()) {
            isDownstreamRetx = true;
            NS_LOG_DEBUG("This is a retransmission interest from downstream, drop it - " << interest->getName().toUri());
            downstreamRetxCount++;
            return;
        }

        // If queue isn't full, perform interest splitting
        if (!isQueueFull && !isDownstreamRetx) {
            // Store original name into aggMap
            m_agg_newDataName[seq] = interest->getName().toUri();

            NS_LOG_DEBUG("New downstream interest's seq: " << seq);

            // Split interest
            InterestSplitting(seq);

            if (firstInterest) {
                for (const auto& [key, value] : aggregationMap) {
                    m_scheduleEvent[key] = Simulator::ScheduleNow(&Aggregator::ScheduleNextPacket, this, key);
                }
                firstInterest = false;
            }
        } else {
            NS_LOG_DEBUG("Error! Interest queue is full or downstream retransmission, please check!");
            Simulator::Stop();
            return;
        }
    } 
    else if (interestType == "initialization") {
        // Synchronize signal
        treeSync = true;

        // Record current time as simulation start time on aggregator
        startSimulation = Simulator::Now();

        // Read aggregation tree from init message
        std::vector<std::string> inputs;
        if (interest->getName().size() > 3) {
            for (size_t i = 1; i < interest->getName().size() - 2; ++i) {
                inputs.push_back(interest->getName().get(i).toUri());
            }
        }
        aggregationMap = aggTreeProcessStrings(inputs);

        // Define for new congestion control
        numChild = static_cast<int> (aggregationMap.size());


        // After receiving aggregation tree, start basic initialization
        // Initialize logging session
        InitializeLogFile();

        // Initialize parameteres
        InitializeParameters();

        // Perform interest name splitting
        InterestGenerator();
        
        //! nack works
/*         auto nack = std::make_shared<ndn::lp::Nack>(*interest);
        nack->setReason(ndn::lp::NackReason::QUEUE_OVERFLOW);
        //nack->wireEncode();
        m_transmittedNacks(nack, this, m_face);
        m_appLink->onReceiveNack(*nack); */

        // Generate a new data packet to respond to tree broadcasting
        Name dataName(interest->getName());
        auto data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

        SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
        if (m_keyLocator.size() > 0) {
            signatureInfo.setKeyLocator(m_keyLocator);
        }
        data->setSignatureInfo(signatureInfo);
        ::ndn::EncodingEstimator estimator;
        ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
        encoder.appendVarNumber(m_signature);
        data->setSignatureValue(encoder.getBuffer());

        // to create real wire encoding
        data->wireEncode();
        m_transmittedDatas(data, this, m_face);
        m_appLink->onReceiveData(*data);
    }
}



void
Aggregator::ScheduleNextPacket(std::string prefix)
{
    if (interestQueue.find(prefix) == interestQueue.end()) {
        NS_LOG_DEBUG("Flow " << prefix << " is not found in the interest queue.");
        Simulator::Stop();
        return;
    }
    
    if (!interestQueue.at(prefix).empty()) {
        if (m_sendEvent[prefix].IsRunning()) {
            Simulator::Remove(m_sendEvent[prefix]);
            NS_LOG_DEBUG("Suspicious, remove the previous event.");
        } 
        m_sendEvent[prefix] = Simulator::ScheduleNow(&Aggregator::SendPacket, this, prefix);

        double nextTime = 1/m_rateLimit[prefix]; // Unit: us
        NS_LOG_INFO("Flow " << prefix << " -> Schedule next sending event after " << nextTime / 1000  << " ms.");
        m_scheduleEvent[prefix] = Simulator::Schedule(MicroSeconds(nextTime), &Aggregator::ScheduleNextPacket, this, prefix);            
    } else{
        // Schedule again after 1/5 rate limit
        double nextTime = 1/m_rateLimit[prefix]/5; // Unit: us
        NS_LOG_INFO("Flow " << prefix << " -> Interest queue is empty. Schedule next sending event after " << nextTime / 1000 << " ms.");
        m_scheduleEvent[prefix] = Simulator::Schedule(MicroSeconds(nextTime), &Aggregator::ScheduleNextPacket, this, prefix);
    }
}



void
Aggregator::InterestGenerator()
{
    // Divide interests and push them into queue
    for (const auto& [key, values] : aggregationMap) {
        std::string name_sec1;
        std::string name_sec0_2;
        
        for (const auto& value : values) {
            name_sec1 += value + ".";
        }
        name_sec1.resize(name_sec1.size() - 1);
        name_sec0_2 = "/" + key + "/" + name_sec1 + "/data";
        NameSec0_2[key] = name_sec0_2;
        vec_iteration.push_back(key); // Will be added to aggregation map later
    }       
}



bool
Aggregator::InterestSplitting(uint32_t seq)
{
    // Divide interests and push them into queue
    for (const auto& [key, value] : aggregationMap) {
        interestQueue[key].push_back(seq);
    }
}



void
Aggregator::SendPacket(std::string prefix)
{
    if (!interestQueue.at(prefix).empty()) {
        uint32_t iteration = interestQueue.at(prefix).front();
        interestQueue.at(prefix).pop_front();
        shared_ptr<Name> name = make_shared<Name>(NameSec0_2[prefix]);
        name->appendSequenceNumber(iteration);

        SendInterest(name);

        // Check whether it's new iteration
        if (aggregateStartTime.find(iteration) == aggregateStartTime.end()) {
            aggregateStartTime[iteration] = Simulator::Now();
            map_agg_oldSeq_newName[iteration] = vec_iteration;
        }

        // Stop interest scheduling after reaching the last iteration
        if (iteration == m_iteNum) {
            NS_LOG_INFO("All iterations have been finished, no need to schedule new interests.");
            if (m_scheduleEvent[prefix].IsRunning()) {
                Simulator::Remove(m_scheduleEvent[prefix]);
            }
        }
    } else {
        NS_LOG_DEBUG("Flow - " << prefix << ": interest queue is empty, this should never happen!");
        Simulator::Stop();
        return;
    }
}



void
Aggregator::SendInterest(shared_ptr<Name> newName)
{
    if (!m_active)
        return;

    std::string nameWithSeq = newName->toUri();
    std::string name_sec0 = newName->get(0).toUri();

    // Trace timeout
    m_timeoutCheck[nameWithSeq] = Simulator::Now();

    // Start response time
    rttStartTime[nameWithSeq] = Simulator::Now();

    NS_LOG_INFO("Sending new interest >>>> " << nameWithSeq);
    shared_ptr<Interest> newInterest = make_shared<Interest>();
    newInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    newInterest->setCanBePrefix(false);
    newInterest->setName(*newName);
    time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
    newInterest->setInterestLifetime(interestLifeTime);
    m_transmittedInterests(newInterest, this, m_face);
    m_appLink->onReceiveInterest(*newInterest);

    // Designed for congestion control recording
    m_inFlight[name_sec0]++;

    // Record interest throughput
    // Actual interests sending and retransmission are recorded as well
    int interestSize = newInterest->wireEncode().size();
    totalInterestThroughput += interestSize;
    NS_LOG_DEBUG("Interest size: " << interestSize);
}



void
Aggregator::SendData(uint32_t seq)
{
    // Get aggregation result for current iteration
    std::vector<uint8_t> newbuffer;
    serializeModelData(GetMean(seq), newbuffer);

    // create data packet
    auto data = make_shared<Data>();

    std::string name_string = m_agg_newDataName[seq];
    NS_LOG_INFO("New aggregated data's name: " << name_string);

    shared_ptr<Name> newName = make_shared<Name>(name_string);
    data->setName(*newName);
    data->setContent(make_shared< ::ndn::Buffer>(newbuffer.begin(), newbuffer.end()));
    data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

    if (m_keyLocator.size() > 0){
        signatureInfo.setKeyLocator(m_keyLocator);
    }
    data->setSignatureInfo(signatureInfo);
    ::ndn::EncodingEstimator estimator;
    ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
    encoder.appendVarNumber(m_signature);
    data->setSignatureValue(encoder.getBuffer());
    data->wireEncode();
    m_transmittedDatas(data, this, m_face);
    m_appLink->onReceiveData(*data);

    // Clear aggregation mapping and aggregation result for current iteration
    aggregateTime.erase(seq);
    map_agg_oldSeq_newName.erase(seq);
    m_agg_newDataName.erase(seq);
    sumParameters.erase(seq);
    partialAggResult.erase(seq);
}



void
Aggregator::SendNack(shared_ptr<const Interest> interest)
{
    auto nack = std::make_shared<ndn::lp::Nack>(*interest);
    nack->setReason(ndn::lp::NackReason::QUEUE_OVERFLOW);
    //nack->wireEncode();
    m_transmittedNacks(nack, this, m_face);
    m_appLink->onReceiveNack(*nack);
}



void
Aggregator::OnData(shared_ptr<const Data> data)
{
    if (!m_active)
        return;

    App::OnData(data);
    NS_LOG_INFO ("Received content object: " << boost::cref(*data));
    int dataSize = data->wireEncode().size();

    std::string dataName = data->getName().toUri();
    std::string name_sec0 = data->getName().get(0).toUri();
    uint32_t seq = data->getName().at(-1).toSequenceNumber();
    std::string type = data->getName().get(-2).toUri();

    // Record data throughput
    totalDataThroughput += dataSize;
    NS_LOG_DEBUG("The incoming data packet size is: " << dataSize);

    // Stop checking timeout associated with this name
    if (m_timeoutCheck.find(dataName) != m_timeoutCheck.end())
        m_timeoutCheck.erase(dataName);
    else {
        NS_LOG_DEBUG("Suspicious data packet, not exists in timeout list.");
        Simulator::Stop();
    }
          
    //? Currently pause the interest sending for 5 * current period
    // Check whether data queue exceeds the limit
    if (sumParameters.find(seq) == sumParameters.end()) {
        // New iteration, currently not exist in the partial agg result
        if (partialAggResult.size() >= m_dataQueue) {
            // Exceed max data size
            NS_LOG_INFO("Exceeding the max data queue, stop interest sending for flow " << name_sec0);
            NS_LOG_INFO("Current partial aggregation table size is: " << partialAggResult.size());
            dataOverflow++;

            // Schdule next event after 5 * current period
            if (m_scheduleEvent[name_sec0].IsRunning()) {
                Simulator::Remove(m_scheduleEvent[name_sec0]);
            }
            
            double nextTime = 5*1/m_rateLimit[name_sec0]; // Unit: us
            NS_LOG_INFO("Flow " << name_sec0 << " -> Schedule next sending event after " << nextTime / 1000  << " ms.");
            m_scheduleEvent[name_sec0] = Simulator::Schedule(MicroSeconds(nextTime), &Aggregator::ScheduleNextPacket, this, name_sec0);
        }
        partialAggResult[seq] = true;
    }

    if (m_inFlight[name_sec0] > 0) {
        m_inFlight[name_sec0]--;
    } else {
        NS_LOG_DEBUG("Error! In-flight packet is less than 0, please check!");
        Simulator::Stop();
        return;
    }

    // Check data type
    if (type == "data") {
        // Perform data name matching with interest name
        ModelData upstreamModelData;

        auto data_agg = map_agg_oldSeq_newName.find(seq);
        auto data_map = m_agg_newDataName.find(seq);
        if (data_map != m_agg_newDataName.end() && data_agg != map_agg_oldSeq_newName.end())
        {
            // Aggregation starts
            auto& vec = data_agg->second;
            auto vecIt = std::find(vec.begin(), vec.end(), name_sec0);
            std::vector<uint8_t> oldbuffer(data->getContent().value(), data->getContent().value() + data->getContent().value_size());
            if (deserializeModelData(oldbuffer, upstreamModelData)) {
                if (vecIt != vec.end())
                {
                    Aggregate(upstreamModelData, seq);
                    vec.erase(vecIt);
                } 
                else
                {
                    NS_LOG_INFO("Data name doesn't exist in aggMap, meaning this data packet is duplicate from upstream!");
                    Simulator::Stop();
                    return;
                }
            } else {
                NS_LOG_INFO("Error when deserializing data packet, please check!");
                Simulator::Stop();
                return;
            }            

            // RTT measurement
            if (rttStartTime.find(dataName) != rttStartTime.end()){
                responseTime[dataName] = Simulator::Now() - rttStartTime[dataName];
                ResponseTimeSum(responseTime[dataName].GetMicroSeconds());
                NS_LOG_INFO("ResponseTime for data packet : " << dataName << "=> is: " << responseTime[dataName].GetMicroSeconds() << " us");
            }

            // RTO/RTT measure
            RTOMeasure(name_sec0, responseTime[dataName].GetMicroSeconds());
            RTTMeasure(name_sec0, responseTime[dataName].GetMicroSeconds());
            
            // Update estimated bandwidth
            BandwidthEstimation(name_sec0);

            // Init rate limit update
            if (firstData.at(name_sec0)) {
                NS_LOG_DEBUG("Init rate limit update for flow " << name_sec0);
                m_rateEvent[name_sec0] = Simulator::ScheduleNow(&Aggregator::RateLimitUpdate, this, name_sec0);
                firstData[name_sec0] = false;
            }

            // Record QueueSize-based CC info
            QueueRecorder(name_sec0, getDataQueueSize(name_sec0));
            
            // Record RTT
            ResponseTimeRecorder(responseTime[dataName], seq, name_sec0);

            // Record RTO
            RTORecorder(name_sec0);
            
            InFlightRecorder(name_sec0);

            // Check whether the aggregation of current iteration is done
            if (vec.empty()){
                NS_LOG_INFO("Aggregation of iteration " << seq << " finished.");

                // Measure aggregation time
                if (aggregateStartTime.find(seq) != aggregateStartTime.end()) {
                    aggregateTime[seq] = Simulator::Now() - aggregateStartTime[seq];
                    AggregateTimeSum(aggregateTime[seq].GetMicroSeconds());
                    aggregateStartTime.erase(seq);
                    NS_LOG_INFO("Aggregator's aggregate time of sequence " << seq << " is: " << aggregateTime[seq].GetMilliSeconds() << " ms");
                } else {
                    NS_LOG_DEBUG("Error when calculating aggregation time, no reference found for seq " << seq);
                }

                // Record aggregation time
                AggregateTimeRecorder(aggregateTime[seq], seq);

                // Send data
                //! Debug, using 1ms instread of 2ms
                //NS_LOG_DEBUG("Send data packet after 2 ms.");
                //Simulator::Schedule(MilliSeconds(2), &Aggregator::SendData, this, seq);
                NS_LOG_DEBUG("Send data packet after 1 ms.");
                Simulator::Schedule(MilliSeconds(1), &Aggregator::SendData, this, seq);

                // All iterations finished, record the entire throughput
                if (iterationCount == m_iteNum) {
                    stopSimulation = Simulator::Now();

                    // Record throughput and results
                    ThroughputRecorder(totalInterestThroughput, totalDataThroughput, startSimulation);
                    ResultRecorder(GetAggregateTimeAverage());
                }

            } else{
                NS_LOG_DEBUG("Wait for others to aggregate.");
            }

            // Clear rtt mapping of this packet
            rttStartTime.erase(dataName);
            responseTime.erase(dataName);

        } else {
            NS_LOG_DEBUG("Error, data name can't be recognized!");
            Simulator::Stop();
            return;
        }
    }
}



void
Aggregator::WindowRecorder(std::string prefix)
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(window_recorder[prefix], std::ios::app);

    if (file.is_open()) {
        file << Simulator::Now().GetMilliSeconds() << " " << m_window[prefix] << " " << m_ssthresh[prefix] << " " << interestQueue[prefix].size() << std::endl;  // Write text followed by a newline
        file.close(); // Close the file after writing
    } else {
        std::cerr << "Unable to open file: " << window_recorder[prefix] << std::endl;
    }
}



void
Aggregator::InFlightRecorder(std::string prefix)
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(inFlight_recorder[prefix], std::ios::app);

    if (file.is_open()) {
        file << Simulator::Now().GetMilliSeconds() << " " << m_inFlight[prefix] << std::endl;  // Write text followed by a newline
        file.close(); // Close the file after writing
    } else {
        std::cerr << "Unable to open file: " << inFlight_recorder[prefix] << std::endl;
    }
}



void
Aggregator::ResponseTimeRecorder(Time responseTime, uint32_t seq, std::string prefix) {
    // Open the file using fstream in append mode
    std::ofstream file(responseTime_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder[prefix] << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << Simulator::Now().GetMilliSeconds() << " " << seq  << " " << responseTime.GetMicroSeconds() / 1000 << std::endl;

    // Close the file
    file.close();
}



void
Aggregator::RTORecorder(std::string prefix)
{
    // Open the file using fstream in append mode
    std::ofstream file(RTO_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << RTO_recorder[prefix] << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << Simulator::Now().GetMilliSeconds() << " " << RTO_threshold[prefix].GetMicroSeconds() / 1000 << std::endl;

    // Close the file
    file.close();
}



void
Aggregator::AggregateTimeRecorder(Time aggregateTime, uint32_t seq) {
    // Open the file using fstream in append mode
    std::ofstream file(aggregateTime_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << aggregateTime_recorder << std::endl;
        return;
    }

    // Write aggregation time to file, followed by a new line
    file << Simulator::Now().GetMilliSeconds() << " " << seq << " " << aggregateTime.GetMicroSeconds() / 1000 << std::endl;

    file.close();
}



void
Aggregator::InitializeLogFile()
{
    // Check whether the object path exists, if not, create it first
    CheckDirectoryExist(folderPath);

    // Open the file and clear all contents for all log files
    // Initialize file name for different upstream node, meaning that RTT/cwnd is measured per flow
    for (const auto& [child, leaves] : aggregationMap) {
        RTO_recorder[child] = folderPath + m_prefix.toUri() + "_RTO_" + child + ".txt";
        responseTime_recorder[child] = folderPath + m_prefix.toUri() + "_RTT_" + child + ".txt";
        //window_recorder[child] = folderPath + m_prefix.toUri() + "_window_" + child + ".txt";
        inFlight_recorder[child] = folderPath + m_prefix.toUri() + "_inFlight_" + child + ".txt";
        qsNew_recorder[child] = folderPath + m_prefix.toUri() + "_queue_" + child + ".txt";
        OpenFile(RTO_recorder[child]);
        OpenFile(responseTime_recorder[child]);
        //OpenFile(window_recorder[child]);
        OpenFile(inFlight_recorder[child]);
        OpenFile(qsNew_recorder[child]);
    }

    // Initialize log file for aggregate time
    aggregateTime_recorder = folderPath + m_prefix.toUri() + "_aggregationTime.txt";
    OpenFile(aggregateTime_recorder);
    //aggTable_recorder = folderPath + m_prefix.toUri() + "_aggTable_.txt";
    //OpenFile(aggTable_recorder);
}




void
Aggregator::InitializeParameters()
{
    // Initialize window
    for (const auto& [key, value] : aggregationMap) {
        m_window[key] = m_initialWindow;
        m_inFlight[key] = 0;
        m_ssthresh[key] = std::numeric_limits<double>::max();
        //successiveCongestion[key] = 0;

        // Initialize RTO measurement parameters
        SRTT[key] = 0;
        RTTVAR[key] = 0;
        roundRTT[key] = 0;

        // Initialize CUBIC factor
        m_cubicLastWmax[key] = m_initialWindow;
        m_cubicWmax[key] = m_initialWindow;

        lastWindowDecreaseTime[key] = Simulator::Now();
        RTT_historical_estimation[key] = 0;
        RTT_count[key] = 0;

        // Initialize timeout checking
        RTO_threshold[key] = 5 * m_retxTimer;

        // Initialize seq
        SeqMap[key] = 0;

        // Initialize QueueSize-based CC info
        m_qsSlidingWindows[key] = utils::SlidingWindow<double>(MilliSeconds(m_qsTimeDuration));
        m_estimatedBW[key] = 0;
        m_rateLimit[key] = m_qsInitRate;
        firstData[key] = true;
        RTT_estimation_qs[key] = 0;
        nackSignal[key] = false;
        timeoutSignal[key] = false;
        m_lastBW[key] = 0;
        m_ccState[key] = "Startup";
        m_inflightLimit[key] = 0;
    }

    // Init params for interest sending rate pacing
    firstInterest = true;
    isRTTEstimated = false;
}



bool
Aggregator::CanDecreaseWindow(std::string prefix, int64_t threshold)
{
    if (Simulator::Now().GetMilliSeconds() - lastWindowDecreaseTime[prefix].GetMilliSeconds() > threshold) {
        return true;
    } else {
        return false;
    }
}



void
Aggregator::ThroughputRecorder(int interestThroughput, int dataThroughput, Time start_simulation)
{
    // Open the file using fstream in append mode
    std::ofstream file(throughput_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << throughput_recorder << std::endl;
        return;
    }

    // Write aggregation time to file, followed by a new line
    file << interestThroughput << " " << dataThroughput << " " << numChild << " " << start_simulation.GetMilliSeconds() << " " << Simulator::Now().GetMilliSeconds() << std::endl;

    file.close();
}



void
Aggregator::ResultRecorder(int64_t aveAggTime)
{
    // Open the file using fstream in append mode
    std::ofstream file(result_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << result_recorder << std::endl;
        return;
    }

    file << m_prefix << "'s result" << std::endl;
    file << "Total iterations: " << m_iteNum << std::endl;
    file << "Timeout is triggered for " << suspiciousPacketCount << " times" << std::endl;
    file << "The number of downstream duplicate interest retransmission is " << downstreamRetxCount << " times" << std::endl;
    file << "Interest queue overflow is triggered for " << interestOverflow << " times" << std::endl;
    file << "Data queue overflow is triggered for " << dataOverflow << " times" << std::endl;
    file << "Nack(upstream interest queue overflow) is triggered for " << nackCount << " times" << std::endl;
    file << "Average aggregation time: " << aveAggTime / 1000  << " ms" << std::endl;
    file << "-----------------------------------" << std::endl;
}



void
Aggregator::QueueRecorder(std::string prefix, double queueSize)
{
    // Open the file using fstream in append mode
    std::ofstream file(qsNew_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << qsNew_recorder[prefix] << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    //* Note that throughput is transferred into Mbps
    file << Simulator::Now().GetMilliSeconds() << " " 
         << m_rateLimit[prefix] * 1000 << " "
         << m_estimatedBW[prefix] * 1000 << " " 
         << GetDataRate(prefix) * 1000000 * 8 * 8 * m_dataSize / 1000000 << " "
         << queueSize << " "
         << m_inFlight[prefix] << " "
         << RTT_estimation_qs[prefix] / 1000 << " "  
         << std::endl;

    // Close the file
    file.close();
}



void
Aggregator::RTTMeasure(std::string prefix, int64_t resTime)
{
    // Update RTT estimation
    if (RTT_estimation_qs[prefix] == 0) {
        RTT_estimation_qs[prefix] = resTime;
    } else {
        RTT_estimation_qs[prefix] = m_EWMAFactor * RTT_estimation_qs[prefix] + (1 - m_EWMAFactor) * resTime;
    }
}



double
Aggregator::GetDataRate(std::string prefix)
{
    double rawDataRate = m_qsSlidingWindows[prefix].GetDataArrivalRate();

    // "0": sliding window size is less than one, keep init rate as data arrival rate; "-1" indicates error
    if (rawDataRate == -1) {
        NS_LOG_INFO("Returned data arrival rate is -1, please check!");
        Simulator::Stop();
        return 0.0;  
    } else if (rawDataRate == 0) {       
        NS_LOG_INFO("Sliding window is not enough, use 0 as data arrival rate: " << " 0 pkgs/ms");
        return 0.0;
    } else {
        return rawDataRate;
    }
}



void
Aggregator::BandwidthEstimation(std::string prefix)
{
    Time arrivalTime = Simulator::Now();
    
    // Update info within sliding window
    double queueSize = getDataQueueSize(prefix);
    NS_LOG_INFO("Flow: " << prefix << ", Data queue size: " << queueSize);
    m_qsSlidingWindows[prefix].AddPacket(arrivalTime, queueSize);

    double aveQS = m_qsSlidingWindows[prefix].GetAverageQueue();

    //double dataArrivalRate = m_qsSlidingWindows[prefix].GetDataRate();
    double dataArrivalRate = GetDataRate(prefix);

    // Update bandwidth estimation
    if (dataArrivalRate == 0) {
        NS_LOG_INFO("Data rate is 0, don't update bandwidth.");
    } else {
        if (aveQS > m_queueThreshold) {
            m_estimatedBW[prefix] = dataArrivalRate;
        }

        if (dataArrivalRate > m_estimatedBW[prefix]) {
            m_estimatedBW[prefix] = dataArrivalRate;
        }        
    }   

    NS_LOG_INFO("Flow: " << prefix << 
                " - Average data queue size: " << aveQS << 
                ", Arrival Rate: " << dataArrivalRate * 1000  << 
                " pkgs/ms, Bandwidth estimation: " << m_estimatedBW[prefix] * 1000  << " pkgs/ms");
}



void
Aggregator::RateLimitUpdate(std::string prefix)
{

    double aveQS = m_qsSlidingWindows[prefix].GetAverageQueue();
    NS_LOG_INFO("Flow " << prefix << " - data queue size: " << aveQS);

    // Congestion control
    if (m_estimatedBW[prefix] != 0) {
        if (nackSignal[prefix]) {
            nackSignal[prefix] = false;
            m_rateLimit[prefix] = m_estimatedBW[prefix] * m_qsMDFactor;
            NS_LOG_INFO("Congestion detected. Reason: nack signal detected. Update rate limit: " << m_rateLimit[prefix] * 1000 << " pkgs/ms");
        } else if (timeoutSignal[prefix]) {
            timeoutSignal[prefix] = false;
            m_rateLimit[prefix] = m_estimatedBW[prefix] * m_qsMDFactor;
            NS_LOG_INFO("Congestion detected. Reason: timeout . Update rate limit: " << m_rateLimit[prefix] * 1000 << " pkgs/ms");
        } else if (aveQS > 2 * m_queueThreshold) {
            m_rateLimit[prefix] = m_estimatedBW[prefix] * m_qsMDFactor;
            NS_LOG_INFO("Congestion detected. Reason: large data queue. Update rate limit: " << m_rateLimit[prefix] * 1000 << " pkgs/ms");
        } else if (m_inFlight[prefix] > 1.5 * m_inflightThreshold) {
            m_rateLimit[prefix] = m_estimatedBW[prefix] * m_qsMDFactor;
            NS_LOG_INFO("Congestion detected. Reason: inflight interests. Update rate limit: " << m_rateLimit[prefix] * 1000 << " pkgs/ms");
        } else {
            m_rateLimit[prefix] = m_estimatedBW[prefix];
            NS_LOG_INFO("No congestion. Update rate limit by estimated BW: " << m_rateLimit[prefix] * 1000  << " pkgs/ms");
        }        
    }

    // Rate probing
    if (aveQS < m_queueThreshold && m_inFlight[prefix] < m_inflightThreshold) {
        m_rateLimit[prefix] = m_rateLimit[prefix] * m_qsRPFactor;
        NS_LOG_INFO("Start rate probing. Updated rate limit: " << m_rateLimit[prefix] * 1000  << " pkgs/ms");
    }
    
    // Error handling
    if (RTT_estimation_qs[prefix] == 0) {
        NS_LOG_INFO("RTT estimation is 0, please check!");
        Simulator::Stop();
        return;
    }
    
    NS_LOG_INFO("Flow " << prefix << " - Schedule next rate limit update after " << static_cast<double>(RTT_estimation_qs[prefix] / 1000) << " ms");
    m_rateEvent[prefix] = Simulator::Schedule(MicroSeconds(RTT_estimation_qs[prefix]), &Aggregator::RateLimitUpdate, this, prefix);
}




} // namespace ndn
} // namespace ns3


