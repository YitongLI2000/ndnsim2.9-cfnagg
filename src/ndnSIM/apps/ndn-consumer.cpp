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
 *
 *
 * Main structure is modified by Yitong, new functions are added for CFNAgg
 * This class implements all functions for CFNAgg, except for cwnd management/congestion control mechanism.
 * Those parts are implemented in "ndn-consumer-INA"
 **/

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>

#include "ndn-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>


#include "src/ndnSIM/apps/algorithm/include/AggregationTree.hpp"
#include "src/ndnSIM/apps/algorithm/utility/utility.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.Consumer");

using namespace std::chrono;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Consumer);



TypeId
Consumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Consumer")
        .SetGroupName("Ndn")
        .SetParent<App>()

        .AddAttribute("StartSeq",
                    "Initial sequence number",
                    IntegerValue(0),
                    MakeIntegerAccessor(&Consumer::m_seq),
                    MakeIntegerChecker<int32_t>())
        .AddAttribute("Prefix",
                    "Name of the Node",
                    StringValue(),
                    MakeNameAccessor(&Consumer::m_prefix),
                    MakeNameChecker())
        .AddAttribute("TopologyType",
                    "Network topology type, DCN/ISP",
                    StringValue(),
                    MakeStringAccessor(&Consumer::m_topologyType),
                    MakeStringChecker())
        .AddAttribute("RTTWindowSize",
                    "RTT window average size",
                    IntegerValue(3),
                    MakeIntegerAccessor(&Consumer::m_smooth_window_size),
                    MakeIntegerChecker<int>())          
        .AddAttribute("NodePrefix",
                    "Node prefix",
                    StringValue(),
                    MakeStringAccessor(&Consumer::m_nodeprefix),
                    MakeStringChecker())
        .AddAttribute("LifeTime",
                    "LifeTime for interest packet",
                    StringValue("4s"),
                    MakeTimeAccessor(&Consumer::m_interestLifeTime),
                    MakeTimeChecker())
        .AddAttribute("EWMAFactor",
                    "EWMA factor used when measuring RTT, recommended between 0.1 and 0.3",
                    DoubleValue(0.3),
                    MakeDoubleAccessor(&Consumer::m_EWMAFactor),
                    MakeDoubleChecker<double>())
        .AddAttribute("CcAlgorithm",
                    "Specify which window adaptation algorithm to use (AIMD, or CUBIC)",
                    EnumValue(CcAlgorithm::AIMD),
                    MakeEnumAccessor(&Consumer::m_ccAlgorithm),
                    MakeEnumChecker(CcAlgorithm::AIMD, "AIMD", CcAlgorithm::CUBIC, "CUBIC"))                     
        .AddAttribute("UseWIS",
                    "Suppress the window increasing rate after congestion",
                    BooleanValue(true),
                    MakeBooleanAccessor(&Consumer::m_useWIS),
                    MakeBooleanChecker())                    
        .AddAttribute("UseCubicFastConv",
                    "If true, use Fast Convergence for Cubic",
                    BooleanValue(false),
                    MakeBooleanAccessor(&Consumer::m_useCubicFastConv),
                    MakeBooleanChecker())                    
        .AddAttribute("ThresholdFactor",
                    "Factor to compute actual RTT threshold",
                    DoubleValue(1.0),
                    MakeDoubleAccessor(&Consumer::m_thresholdFactor),
                    MakeDoubleChecker<double>())
        .AddAttribute("Iteration",
                    "The number of iterations to run in the simulation",
                    IntegerValue(200),
                    MakeIntegerAccessor(&Consumer::m_iteNum),
                    MakeIntegerChecker<int32_t>())
        .AddAttribute("InterestQueueSize",
                    "The size of interest queue",
                    IntegerValue(5),
                    MakeIntegerAccessor(&Consumer::m_interestQueue),
                    MakeIntegerChecker<int>())
        .AddAttribute("DataQueueSize",
                    "The size of interest queue",
                    IntegerValue(5),
                    MakeIntegerAccessor(&Consumer::m_dataQueue),
                    MakeIntegerChecker<int>())
        .AddAttribute("Constraint",
                    "Constraint of aggregation tree construction",
                    IntegerValue(5),
                    MakeIntegerAccessor(&Consumer::m_constraint),
                    MakeIntegerChecker<int>())
        .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("10ms"),
                    MakeTimeAccessor(&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
                    MakeTimeChecker())
        .AddAttribute("DataSize",
                    "Define the data content size",
                    IntegerValue(150),
                    MakeIntegerAccessor(&Consumer::m_dataSize),
                    MakeIntegerChecker<int>())    
        .AddAttribute("InitPace",
                    "Initial size of the interest sending pace, default is 2 ms",
                    IntegerValue(2),
                    MakeIntegerAccessor(&Consumer::m_initPace),
                    MakeIntegerChecker<int>())
        .AddAttribute("ConQueueThreshold",
                    "QueueSize-based CC's queue threshold",
                    IntegerValue(10),
                    MakeIntegerAccessor(&Consumer::m_queueThreshold),
                    MakeIntegerChecker<int>())
        .AddAttribute("InFlightThreshold",
                    "Inflight threshold",
                    IntegerValue(20),
                    MakeIntegerAccessor(&Consumer::m_inflightThreshold),
                    MakeIntegerChecker<int>())                    
        .AddAttribute("QSMDFactor",
                    "QueueSize-based CC's multiplicative decrease factor",
                    DoubleValue(0.9),
                    MakeDoubleAccessor(&Consumer::m_qsMDFactor),
                    MakeDoubleChecker<double>())
        .AddAttribute("QSRPFactor",
                    "QueueSize-based CC's rate probing factor",
                    DoubleValue(1.05),
                    MakeDoubleAccessor(&Consumer::m_qsRPFactor),
                    MakeDoubleChecker<double>())                   
        .AddAttribute("QSSlidingWindowDuration",
                    "QueueSize-based CC's sliding window size",
                    IntegerValue(5),
                    MakeIntegerAccessor(&Consumer::m_qsTimeDuration),
                    MakeIntegerChecker<int>())                        
        .AddAttribute("QSInitRate",
                    "QueueSize-based CC's initial interest sending rate",
                    DoubleValue(0.5),
                    MakeDoubleAccessor(&Consumer::m_qsInitRate),
                    MakeDoubleChecker<double>());                    
    return tid;
}



Consumer::Consumer()
    : suspiciousPacketCount(0)
    , dataOverflow(0)
    , nackCount(0)
    , totalInterestThroughput(0)
    , totalDataThroughput(0)
    , throughputStable(false)
    , linkCount(0)
    , initSeq(0)
    , globalSeq(0)
    , broadcastSync(false)
    , m_rand(CreateObject<UniformRandomVariable>())
    , m_seq(0)
    , total_response_time(0)
    , round(0)
    , totalAggregateTime(0)
    , iterationCount(0)
    , m_minWindow(1)
{
    m_rtt = CreateObject<RttMeanDeviation>();
}



void
Consumer::TreeBroadcast()
{
    const auto& broadcastTree = aggregationTree[0];

    for (const auto& [parentNode, childList] : broadcastTree) {
        // Don't broadcast to itself
        if (parentNode == m_nodeprefix) {
            continue;
        }

        std::string nameWithType;
        std::string nameType = "initialization";
        nameWithType += "/" + parentNode;
        auto result = getLeafNodes(parentNode, broadcastTree);

        // Construct nameWithType variable for tree broadcast
        for (const auto& [childNode, leaves] : result) {
            std::string name_indication;
            name_indication += childNode + ".";
            for (const auto& leaf : leaves) {
                name_indication += leaf + ".";
            }
            name_indication.resize(name_indication.size() - 1);
            nameWithType += "/" + name_indication;
        }
        nameWithType += "/" + nameType;

        std::cout << "Node " << parentNode << "'s name is: " << nameWithType << std::endl;
        shared_ptr<Name> newName = make_shared<Name>(nameWithType);
        newName->appendSequenceNumber(initSeq);
        SendInterest(newName);
    }
    initSeq++;
}



void
Consumer::ConstructAggregationTree()
{
    App::ConstructAggregationTree();

    // Choose the corresponding topology type
    if (m_topologyType == "DCN") {
        filename = "src/ndnSIM/examples/topologies/DataCenterTopology.txt";
    } else if (m_topologyType == "ISP") {
        filename = "src/ndnSIM/examples/topologies/ISPTopology.txt";
    } else if (m_topologyType == "BinaryTree8") {
        filename = "src/ndnSIM/examples/topologies/BinaryTreeTopology8.txt";
    } else if (m_topologyType == "BinaryTree16") {
        filename = "src/ndnSIM/examples/topologies/BinaryTreeTopology16.txt";
    } else if (m_topologyType == "BinaryTree32") {
        filename = "src/ndnSIM/examples/topologies/BinaryTreeTopology32.txt";
    } else {
        NS_LOG_DEBUG("Topology type error, please check!");
        Simulator::Stop();
        return;
    }

/*     AggregationTree tree(filename);
    std::vector<std::string> dataPointNames = Utility::getProducers(filename);
    std::map<std::string, std::vector<std::string>> rawAggregationTree;
    std::vector<std::vector<std::string>> rawSubTree;

    if (tree.aggregationTreeConstruction(dataPointNames, m_constraint)) {
        rawAggregationTree = tree.aggregationAllocation;
        rawSubTree = tree.noCHTree;
    } else {
        NS_LOG_DEBUG("Fail to construct aggregation tree!");
        Simulator::Stop();
    }

    // Get the number of producers
    producerCount = Utility::countProducers(filename);

    // Create producer list
    for (const auto& item : dataPointNames) {
        proList += item + ".";
    }
    proList.resize(proList.size() - 1); */

    //! Debugging binary tree
    std::vector<std::string> dataPointNames = Utility::getProducers(filename);
    std::map<std::string, std::vector<std::string>> rawAggregationTree;
    std::vector<std::vector<std::string>> rawSubTree;
    
    // Binary tree with forwarders
/*     if (m_topologyType == "BinaryTree8") {
        rawAggregationTree["agg0"] = {"pro0", "pro1"};
        rawAggregationTree["agg1"] = {"pro2", "pro3"};
        rawAggregationTree["agg2"] = {"pro4", "pro5"};
        rawAggregationTree["agg3"] = {"pro6", "pro7"};
        rawAggregationTree["agg4"] = {"agg0", "agg1"};
        rawAggregationTree["agg5"] = {"agg2", "agg3"};
        rawAggregationTree["con0"] = {"agg4", "agg5"};    
    } else if (m_topologyType == "BinaryTree16") {
        rawAggregationTree["agg0"] = {"pro0", "pro1"};
        rawAggregationTree["agg1"] = {"pro2", "pro3"};
        rawAggregationTree["agg2"] = {"pro4", "pro5"};
        rawAggregationTree["agg3"] = {"pro6", "pro7"};
        rawAggregationTree["agg4"] = {"pro8", "pro9"};
        rawAggregationTree["agg5"] = {"pro10", "pro11"};
        rawAggregationTree["agg6"] = {"pro12", "pro13"};
        rawAggregationTree["agg7"] = {"pro14", "pro15"};
        rawAggregationTree["agg8"] = {"agg0", "agg1"};  
        rawAggregationTree["agg9"] = {"agg2", "agg3"};
        rawAggregationTree["agg10"] = {"agg4", "agg5"};  
        rawAggregationTree["agg11"] = {"agg6", "agg7"};
        rawAggregationTree["agg12"] = {"agg8", "agg9"};  
        rawAggregationTree["agg13"] = {"agg10", "agg11"};        
        rawAggregationTree["con0"] = {"agg12", "agg13"};         
    } else if (m_topologyType == "BinaryTree32") {
        rawAggregationTree["agg0"] = {"pro0", "pro1"};
        rawAggregationTree["agg1"] = {"pro2", "pro3"};
        rawAggregationTree["agg2"] = {"pro4", "pro5"};
        rawAggregationTree["agg3"] = {"pro6", "pro7"};
        rawAggregationTree["agg4"] = {"pro8", "pro9"};
        rawAggregationTree["agg5"] = {"pro10", "pro11"};
        rawAggregationTree["agg6"] = {"pro12", "pro13"};
        rawAggregationTree["agg7"] = {"pro14", "pro15"};
        rawAggregationTree["agg8"] = {"pro16", "pro17"};
        rawAggregationTree["agg9"] = {"pro18", "pro19"};
        rawAggregationTree["agg10"] = {"pro20", "pro21"};
        rawAggregationTree["agg11"] = {"pro22", "pro23"};
        rawAggregationTree["agg12"] = {"pro24", "pro25"};
        rawAggregationTree["agg13"] = {"pro26", "pro27"};
        rawAggregationTree["agg14"] = {"pro28", "pro29"};
        rawAggregationTree["agg15"] = {"pro30", "pro31"};     
        rawAggregationTree["agg16"] = {"agg0", "agg1"};  
        rawAggregationTree["agg17"] = {"agg2", "agg3"};
        rawAggregationTree["agg18"] = {"agg4", "agg5"};  
        rawAggregationTree["agg19"] = {"agg6", "agg7"};
        rawAggregationTree["agg20"] = {"agg8", "agg9"};  
        rawAggregationTree["agg21"] = {"agg10", "agg11"};
        rawAggregationTree["agg22"] = {"agg12", "agg13"};  
        rawAggregationTree["agg23"] = {"agg14", "agg15"};
        rawAggregationTree["agg24"] = {"agg16", "agg17"};  
        rawAggregationTree["agg25"] = {"agg18", "agg19"};
        rawAggregationTree["agg26"] = {"agg20", "agg21"};  
        rawAggregationTree["agg27"] = {"agg22", "agg23"};
        rawAggregationTree["agg28"] = {"agg24", "agg25"};  
        rawAggregationTree["agg29"] = {"agg26", "agg27"};        
        rawAggregationTree["con0"] = {"agg28", "agg29"};         
    } else {
        AggregationTree tree(filename);
        if (tree.aggregationTreeConstruction(dataPointNames, m_constraint)) {
            rawAggregationTree = tree.aggregationAllocation;
            rawSubTree = tree.noCHTree;
        } else {
            NS_LOG_DEBUG("Fail to construct aggregation tree!");
            Simulator::Stop();
        }
    } */

    // Binary tree without forwarders
    if (m_topologyType == "BinaryTree8") {
        rawAggregationTree["agg2"] = {"pro0", "pro1"};
        rawAggregationTree["agg3"] = {"pro2", "pro3"};
        rawAggregationTree["agg4"] = {"pro4", "pro5"};
        rawAggregationTree["agg5"] = {"pro6", "pro7"};
        rawAggregationTree["agg0"] = {"agg2", "agg3"};
        rawAggregationTree["agg1"] = {"agg4", "agg5"};
        rawAggregationTree["con0"] = {"agg0", "agg1"};    
    } else if (m_topologyType == "BinaryTree16") {
        rawAggregationTree["agg6"] = {"pro0", "pro1"};
        rawAggregationTree["agg7"] = {"pro2", "pro3"};
        rawAggregationTree["agg8"] = {"pro4", "pro5"};
        rawAggregationTree["agg9"] = {"pro6", "pro7"};
        rawAggregationTree["agg10"] = {"pro8", "pro9"};
        rawAggregationTree["agg11"] = {"pro10", "pro11"};
        rawAggregationTree["agg12"] = {"pro12", "pro13"};
        rawAggregationTree["agg13"] = {"pro14", "pro15"};
        rawAggregationTree["agg2"] = {"agg6", "agg7"};  
        rawAggregationTree["agg3"] = {"agg8", "agg9"};
        rawAggregationTree["agg4"] = {"agg10", "agg11"};  
        rawAggregationTree["agg5"] = {"agg12", "agg13"};
        rawAggregationTree["agg0"] = {"agg2", "agg3"};  
        rawAggregationTree["agg1"] = {"agg4", "agg5"};        
        rawAggregationTree["con0"] = {"agg0", "agg1"};         
    } else if (m_topologyType == "BinaryTree32") {
        rawAggregationTree["agg14"] = {"pro0", "pro1"};
        rawAggregationTree["agg15"] = {"pro2", "pro3"};
        rawAggregationTree["agg16"] = {"pro4", "pro5"};
        rawAggregationTree["agg17"] = {"pro6", "pro7"};
        rawAggregationTree["agg18"] = {"pro8", "pro9"};
        rawAggregationTree["agg19"] = {"pro10", "pro11"};
        rawAggregationTree["agg20"] = {"pro12", "pro13"};
        rawAggregationTree["agg21"] = {"pro14", "pro15"};
        rawAggregationTree["agg22"] = {"pro16", "pro17"};
        rawAggregationTree["agg23"] = {"pro18", "pro19"};
        rawAggregationTree["agg24"] = {"pro20", "pro21"};
        rawAggregationTree["agg25"] = {"pro22", "pro23"};
        rawAggregationTree["agg26"] = {"pro24", "pro25"};
        rawAggregationTree["agg27"] = {"pro26", "pro27"};
        rawAggregationTree["agg28"] = {"pro28", "pro29"};
        rawAggregationTree["agg29"] = {"pro30", "pro31"};   
        rawAggregationTree["agg6"] = {"agg14", "agg15"};  
        rawAggregationTree["agg7"] = {"agg16", "agg17"};
        rawAggregationTree["agg8"] = {"agg18", "agg19"};  
        rawAggregationTree["agg9"] = {"agg20", "agg21"};
        rawAggregationTree["agg10"] = {"agg22", "agg23"};  
        rawAggregationTree["agg11"] = {"agg24", "agg25"};
        rawAggregationTree["agg12"] = {"agg26", "agg27"};  
        rawAggregationTree["agg13"] = {"agg28", "agg29"}; 
        rawAggregationTree["agg2"] = {"agg6", "agg7"};  
        rawAggregationTree["agg3"] = {"agg8", "agg9"};
        rawAggregationTree["agg4"] = {"agg10", "agg11"};  
        rawAggregationTree["agg5"] = {"agg12", "agg13"};
        rawAggregationTree["agg0"] = {"agg2", "agg3"};  
        rawAggregationTree["agg1"] = {"agg4", "agg5"};        
        rawAggregationTree["con0"] = {"agg0", "agg1"};         
    } else {
        AggregationTree tree(filename);
        if (tree.aggregationTreeConstruction(dataPointNames, m_constraint)) {
            rawAggregationTree = tree.aggregationAllocation;
            rawSubTree = tree.noCHTree;
        } else {
            NS_LOG_DEBUG("Fail to construct aggregation tree!");
            Simulator::Stop();
        }
    }


    // Get the number of producers
    producerCount = Utility::countProducers(filename);

    // Create producer list
    for (const auto& item : dataPointNames) {
        proList += item + ".";
    }
    proList.resize(proList.size() - 1);    


    std::cout << "\nAggregation tree: " << std::endl;
    for (const auto& pair : rawAggregationTree) {
        std::cout << pair.first << ": ";
        for (const auto& item : pair.second) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\nSub Tree without CH: " << std::endl;
    for (const auto& pair : rawSubTree) {
        for (const auto& item : pair) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

    // Create aggregationTree， combine different data structure into a complete one
    aggregationTree.push_back(rawAggregationTree);
    while (!rawSubTree.empty()) {
        const auto& item = rawSubTree[0];
        rawAggregationTree[m_nodeprefix] = item;
        aggregationTree.push_back(rawAggregationTree);
        rawSubTree.erase(rawSubTree.begin());
    }


    int i = 0;
    std::cout << "\nIterate all aggregation tree (including main tree and sub-trees)." << std::endl;
    for (const auto& map : aggregationTree) {
        for (const auto& pair : map) {
            std::cout << pair.first << ": ";
            for (const auto& value : pair.second) {
                std::cout << value << " ";
            }
            std::cout << std::endl;

            // 1. Aggregator
            // Initialize "broadcastList" for tree broadcasting synchronization
            if (pair.first != m_nodeprefix) {
                broadcastList.insert(pair.first); // Defined using set, no duplicate elements
            }
            // 2. Consumer
            // Initialize "globalTreeRound" for all rounds (if there're multiple sub-trees)
            else {
                std::vector<std::string> leavesRound;
                std::cout << "Round " << i << " has the following leaf nodes: ";
                for (const auto& leaves : pair.second) {
                    leavesRound.push_back(leaves);
                    std::cout << leaves << " ";
                }
                globalTreeRound.push_back(leavesRound); // Initialize "globalTreeRound"
                std::cout << std::endl;
            }
        }
        std::cout << "----" << std::endl;  // Separator between maps
        i++;
    }

    // Compute link count
    for (const auto& a : globalTreeRound) {
        for (const auto& b : a) {
            linkCount++;
        }
    }
 }



void
Consumer::StartApplication() // Called at time specified by Start
{
    App::StartApplication();

    // Construct the tree
    ConstructAggregationTree();

    // Init log file and parameters
    InitializeLogFile();
    InitializeParameter();

    // Broadcast the tree
    TreeBroadcast();

    // Start interest generator
    InterestGenerator();
}



void
Consumer::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION_NOARGS();
    // cancel periodic packet generation
    //Simulator::Cancel(m_sendEvent);
    App::StopApplication();
}



std::map<std::string, std::set<std::string>>
Consumer::getLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap)
{
    return App::getLeafNodes(key, treeMap);
}



int
Consumer::findRoundIndex(const std::string& target)
{
    return App::findRoundIndex(globalTreeRound, target);
}



double 
Consumer::getDataQueueSize(std::string prefix)
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
Consumer::Aggregate(const ModelData& data, const uint32_t& seq)
{
    // first initialization
    if (sumParameters.find(seq) == sumParameters.end()){
        sumParameters[seq] = std::vector<double>(m_dataSize, 0.0f);
    }

    // Aggregate data
    std::transform(sumParameters[seq].begin(), sumParameters[seq].end(), data.parameters.begin(), sumParameters[seq].begin(), std::plus<double>());
}



std::vector<double>
Consumer::getMean(const uint32_t& seq)
{
    std::vector<double> result;
    if (sumParameters[seq].empty() || producerCount == 0) {
        NS_LOG_DEBUG("Error when calculating average model, please check!");
        return result;
    }

    for (auto value : sumParameters[seq]) {
        result.push_back(value / static_cast<double>(producerCount));
    }

    return result;
}



void
Consumer::ResponseTimeSum (int64_t response_time)
{
    total_response_time += response_time;
    ++round;
}



int64_t
Consumer::GetResponseTimeAverage() {
    if (round == 0){
        NS_LOG_DEBUG("Error happened when calculating response time!");
        return 0;
    }
    return total_response_time / round;
}



void
Consumer::AggregateTimeSum (int64_t aggregate_time)
{
    totalAggregateTime += aggregate_time;
    //NS_LOG_DEBUG("totalAggregateTime is: " << totalAggregateTime);
    ++iterationCount;
}



int64_t
Consumer::GetAggregateTimeAverage()
{
    if (iterationCount == 0)
    {
        NS_LOG_INFO("Error happened when calculating aggregate time!");
        Simulator::Stop();
        return 0;
    }

    // Use milliseconds
    return totalAggregateTime / iterationCount / 1000;
}



void
Consumer::OnNack(shared_ptr<const lp::Nack> nack)
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
Consumer::OnTimeout(std::string nameString)
{
    NS_LOG_INFO("Timeout triggered for: " << nameString);
    shared_ptr<Name> name = make_shared<Name>(nameString);
    std::string name_sec0 = name->get(0).toUri();
    uint32_t seq = name->get(-1).toSequenceNumber();

    if (m_inFlight[name_sec0] > 0) {
        m_inFlight[name_sec0]--;
    } else if (m_inFlight[name_sec0] < 0) {
        NS_LOG_DEBUG("InFlight number error, please exit and check!");
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
Consumer::SetRetxTimer(Time retxTimer)
{
    m_retxTimer = retxTimer;
    if (m_retxEvent.IsRunning()) {
        Simulator::Remove(m_retxEvent); // slower, but better for memory
    }

    // Schedule timeout check event
    NS_LOG_DEBUG("Next interval to check timeout is: " << m_retxTimer.GetMilliSeconds() << " ms");
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}



Time
Consumer::GetRetxTimer() const
{
  return m_retxTimer;
}



void
Consumer::CheckRetxTimeout()
{
    Time now = Simulator::Now();

    for (auto it = m_timeoutCheck.begin(); it != m_timeoutCheck.end();){
        // Parse the string and extract the first segment, e.g. "agg0", then find out its round
        std::string type = make_shared<Name>(it->first)->get(-2).toUri();

        // For "initialization", check timeout by 3 * m_retxTimer
        if (type == "initialization") {
            if (now - it->second > (3 * m_retxTimer)) {
                std::string name = it->first;
                it = m_timeoutCheck.erase(it);
                OnTimeout(name);
            } else {
                ++it;
            }
        } 
        // For "data", check timeout based on measured RTO
        else if (type == "data") {
            std::string name = it->first;
            std::string prefix = make_shared<Name>(name)->get(0).toUri();

            if (now - it->second > RTO_threshold[prefix]) {
                it = m_timeoutCheck.erase(it);
                numTimeout[prefix]++;
                OnTimeout(name);
            } else {
                ++it;
            }
        }
    }
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}



void
Consumer::RTOMeasure(int64_t resTime, std::string prefix)
{
    if (!initRTO[prefix]) {
        RTTVAR[prefix] = resTime / 2;
        SRTT[prefix] = resTime;
        NS_LOG_DEBUG("Initialize RTO for flow: " << prefix);
        initRTO[prefix] = true;
    } else {
        RTTVAR[prefix] = 0.75 * RTTVAR[prefix] + 0.25 * std::abs(SRTT[prefix] - resTime); // RTTVAR = (1 - b) * RTTVAR + b * |SRTT - RTTsample|, where b = 0.25
        SRTT[prefix] = 0.875 * SRTT[prefix] + 0.125 * resTime; // SRTT = (1 - a) * SRTT + a * RTTsample, where a = 0.125
    }
    int64_t RTO = SRTT[prefix] + 4 * RTTVAR[prefix]; // RTO = SRTT + K * RTTVAR, where K = 4

    //RTO_threshold[prefix] = MicroSeconds(4 * RTO);
    RTO_threshold[prefix] = MicroSeconds(2 * RTO);
}



void
Consumer::InterestGenerator()
{
    // Generate name from section 1 to 3 (except for seq)
    std::vector<std::string> objectProducer;
    std::string token;
    std::istringstream tokenStream(proList);
    char delimiter = '.';
    while (std::getline(tokenStream, token, delimiter)) {
        objectProducer.push_back(token);
    }

    for (const auto& aggTree : aggregationTree) {
        auto initialAllocation = getLeafNodes(m_nodeprefix, aggTree); // example - {agg0: [pro0, pro1]}

        for (const auto& [child, leaves] : initialAllocation) {
            std::string name_sec0_2;
            std::string name_sec1;

            for (const auto& leaf : leaves) {
                name_sec1 += leaf + ".";
            }
            name_sec1.resize(name_sec1.size() - 1);
            name_sec0_2 = "/" + child + "/" + name_sec1 + "/data";
            NameSec0_2[child] = name_sec0_2;

            vec_iteration.push_back(child); // Will be added to aggregation map later
        }
    }        
}



bool
Consumer::InterestSplitting()
{   
    bool canSplit = true;
    for (const auto& [prefix, queue] : interestQueue) {
        if (queue.size() >= m_interestQueue) {
            canSplit = false;
            break;
        }
    }

    if (canSplit) {
        // Update seq
        globalSeq++;
        for (auto& [prefix, queue] : interestQueue) {
            queue.push_back(globalSeq);
        }
    } else {
        NS_LOG_INFO("Interest queue is full.");
        return false;
    }

    return true;
}



void
Consumer::SendPacket(std::string prefix)
{
    // Error handling for queue
    if (interestQueue[prefix].empty()) {
        NS_LOG_INFO("No more Interests to send - prefix " << prefix);
        Simulator::Stop();
        return;  // Early return if the queue is empty to avoid popping from an empty deque
    }

    uint32_t seq = interestQueue[prefix].front();
    interestQueue[prefix].pop_front();
    SeqMap[prefix] = seq;

    shared_ptr<Name> newName = make_shared<Name>(NameSec0_2[prefix]);
    newName->appendSequenceNumber(seq);
    NS_LOG_INFO("Sending packet - " << newName->toUri());

    SendInterest(newName);

    // Check whether it's the start of a new iteration
    if (aggregateStartTime.find(seq) == aggregateStartTime.end()) {
        aggregateStartTime[seq] = Simulator::Now();
        map_agg_oldSeq_newName[seq] = vec_iteration;
    }
}



void 
Consumer::SendInterest(shared_ptr<Name> newName)
{
    if (!m_active)
        return;

    std::string nameWithSeq = newName->toUri();
    std::string name_sec0 = newName->get(0).toUri();

    // Trace timeout
    m_timeoutCheck[nameWithSeq] = Simulator::Now();


    rttStartTime[nameWithSeq] = Simulator::Now();

    shared_ptr<Interest> interest = make_shared<Interest>();
    interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    interest->setName(*newName);
    interest->setCanBePrefix(false);
    time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
    interest->setInterestLifetime(interestLifeTime);
    NS_LOG_INFO("Sending interest >>>>" << nameWithSeq);
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);

    m_inFlight[name_sec0]++;
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////
void
Consumer::OnData(shared_ptr<const Data> data)
{
    if (!m_active)
        return;

    App::OnData(data); // tracing inside
    std::string type = data->getName().get(-2).toUri();
    std::string name_sec0 = data->getName().get(0).toUri();
    uint32_t seq = data->getName().at(-1).toSequenceNumber();
    std::string dataName = data->getName().toUri();
    int dataSize = data->wireEncode().size();
    NS_LOG_INFO("Received content object: " << boost::cref(*data));

    // Check whether this's duplicate data packet
    if (m_agg_finished.find(seq) != m_agg_finished.end()) {
        NS_LOG_DEBUG("This data packet is duplicate, stop and check!");
        Simulator::Stop();
    }
    
    //? Currently pause the interest sending for 5 * current period
    // Check partial aggregation table
    if (sumParameters.find(seq) == sumParameters.end()) {
        // New iteration, currently not exist in the partial agg result
        if (partialAggResult.size() >= m_dataQueue) {
            // Exceed max data size
            NS_LOG_INFO("Exceeding the max partial aggregation table, stop interest sending for flow " << name_sec0);
            NS_LOG_INFO("Current partial aggregation table size is: " << partialAggResult.size());
            dataOverflow++;
            
            // Schdule next event after 5 * current period
            if (m_scheduleEvent[name_sec0].IsRunning()) {
                Simulator::Remove(m_scheduleEvent[name_sec0]);
            }
            
            double nextTime = 5*1/m_rateLimit[name_sec0]; // Unit: us
            NS_LOG_INFO("Flow " << name_sec0 << " -> Schedule next sending event after " << nextTime / 1000 << " ms.");
            m_scheduleEvent[name_sec0] = Simulator::Schedule(MicroSeconds(nextTime), &Consumer::ScheduleNextPacket, this, name_sec0);
        }
        partialAggResult[seq] = true;
    }

    // Erase timeout
    if (m_timeoutCheck.find(dataName) != m_timeoutCheck.end())
        m_timeoutCheck.erase(dataName);
    else {
        NS_LOG_DEBUG("Suspicious data packet, not exists in timeout list.");
        Simulator::Stop();
        return;
    }

    if (m_inFlight[name_sec0] > 0) {
        m_inFlight[name_sec0]--;
    }


    if (type == "data") {
        ModelData modelData;

        auto data_agg = map_agg_oldSeq_newName.find(seq);
        if (data_agg != map_agg_oldSeq_newName.end()) {
            // Aggregation starts
            auto& aggVec = data_agg->second;
            auto aggVecIt = std::find(aggVec.begin(), aggVec.end(), name_sec0);
            std::vector<uint8_t> oldbuffer(data->getContent().value(), data->getContent().value() + data->getContent().value_size());

            if (deserializeModelData(oldbuffer, modelData)) {
                if (aggVecIt != aggVec.end())
                {
                    Aggregate(modelData, seq);
                    aggVec.erase(aggVecIt);
                } 
                else
                {
                    NS_LOG_INFO("This data packet is duplicate, error!");
                    Simulator::Stop();
                    return;
                }                
            } else {
                NS_LOG_DEBUG("Error when deserializing data packet, please check!");
                Simulator::Stop();
                return;
            }


            // RTT measurement
            if (rttStartTime.find(dataName) != rttStartTime.end()){
                responseTime[dataName] = Simulator::Now() - rttStartTime[dataName];
                ResponseTimeSum(responseTime[dataName].GetMicroSeconds());
                rttStartTime.erase(dataName);
                NS_LOG_INFO("Consumer's response time of sequence " << dataName << " is: " << responseTime[dataName].GetMilliSeconds() << " ms.");
            }

            // RTO/RTT measure
            RTOMeasure(responseTime[dataName].GetMicroSeconds(), name_sec0);
            RTTMeasure(name_sec0, responseTime[dataName].GetMicroSeconds());

            // Update estimated bandwidth
            BandwidthEstimation(name_sec0);

            // Init rate limit update
            if (firstData.at(name_sec0)) {
                NS_LOG_DEBUG("Init rate limit update for flow " << name_sec0);
                m_rateEvent[name_sec0] = Simulator::ScheduleNow(&Consumer::RateLimitUpdate, this, name_sec0);
                firstData[name_sec0] = false;
            }

            // Record QueueSize-based CC info
            QueueRecorder(name_sec0, getDataQueueSize(name_sec0));

            // Record RTT
            ResponseTimeRecorder(name_sec0, seq, responseTime[dataName]);

            // Record RTO
            RTORecorder(name_sec0);

            InFlightRecorder(name_sec0);

            // Check whether the aggregation iteration has finished
            if (aggVec.empty()) {
                NS_LOG_INFO("Aggregation of iteration " << seq << " finished!");
                std::cout << "Aggregation of iteration " << seq << " finished!" << std::endl;

                // Measure aggregation time
                if (aggregateStartTime.find(seq) != aggregateStartTime.end()) {
                    aggregateTime[seq] = Simulator::Now() - aggregateStartTime[seq];
                    AggregateTimeSum(aggregateTime[seq].GetMicroSeconds());
                    NS_LOG_INFO("Iteration " << seq << "'s aggregation time is: " << aggregateTime[seq].GetMilliSeconds() << " ms.");
                    aggregateStartTime.erase(seq);
                } else {
                    NS_LOG_DEBUG("Error when calculating aggregation time, no reference found for seq " << seq);
                }

                // Record aggregation time
                AggregateTimeRecorder(aggregateTime[seq], seq);

                // Get aggregation result and store them
                aggregationResult[seq] = getMean(seq);

                // Mark the map that current iteration has finished
                m_agg_finished[seq] = true;

                // Clear aggregation time mapping for current iteration
                aggregateTime.erase(seq);                

                // Remove seq from aggMap
                map_agg_oldSeq_newName.erase(seq);
                partialAggResult.erase(seq);
            }

            // Stop simulation
            if (iterationCount == m_iteNum) {
                stopSimulation = Simulator::Now();

                NS_LOG_DEBUG("Reach " << m_iteNum << " iterations, stop!");
                NS_LOG_INFO("Timeout is triggered " << suspiciousPacketCount << " times.");
                //NS_LOG_INFO("Total interest throughput is: " << totalInterestThroughput << " bytes.");
                //NS_LOG_INFO("Total data throughput is: " << totalDataThroughput << " bytes.");
                NS_LOG_INFO("The average aggregation time of Consumer in " << iterationCount << " iteration is: " << GetAggregateTimeAverage() << " ms");

                // Record throughput into file
                //ThroughputRecorder(totalInterestThroughput, totalDataThroughput, startSimulation, startThroughputMeasurement);

                // Record result into file
                int64_t totalTime = Simulator::Now().GetMicroSeconds() - 1000000;
                ResultRecorder(m_iteNum, suspiciousPacketCount, GetAggregateTimeAverage(), totalTime);

                // Stop simulation
                Simulator::Stop();
                return;
            }

            // Clear rtt mapping of this packet
            rttStartTime.erase(dataName);
            responseTime.erase(dataName);
        } else {
            NS_LOG_DEBUG("Suspicious data packet, not exist in aggregation map.");
            Simulator::Stop();
        }
        
    } else if (type == "initialization") {
        // Update synchronization info
        auto it = std::find(broadcastList.begin(), broadcastList.end(), name_sec0);
        if (it != broadcastList.end()) {
            broadcastList.erase(it);
            NS_LOG_DEBUG("Node " << name_sec0 << " has received aggregationTree map, erase it from broadcastList");
        }

        // Tree broadcasting synchronization is done
        if (broadcastList.empty()) {
            broadcastSync = true;
            NS_LOG_DEBUG("Synchronization of tree broadcasting finished!");

            // Record aggregation tree into file
            AggTreeRecorder();
        }

        //* Schedule all flows together after synchronization
        if (broadcastSync) {
            for (const auto& vec_round : globalTreeRound) {
                for (const auto& flow : vec_round) {
                    m_scheduleEvent[flow] = Simulator::ScheduleNow(&Consumer::ScheduleNextPacket, this, flow);
                }
            }
        }
    }
}



bool
Consumer::CongestionDetection(std::string prefix, int64_t responseTime)
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
        NS_LOG_DEBUG("m_smooth_window_size: " << m_smooth_window_size);
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
Consumer::RTORecorder(std::string prefix)
{
    // Open the file using fstream in append mode
    std::ofstream file(RTO_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << RTO_recorder[prefix] << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << Simulator::Now().GetMilliSeconds() << " " << RTO_threshold[prefix].GetMilliSeconds() << std::endl;

    // Close the file
    file.close();
}



void
Consumer::ResponseTimeRecorder(std::string prefix, uint32_t seq, Time responseTime) {
    
    // Open the file using fstream in append mode
    std::ofstream file(responseTime_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder[prefix] << std::endl;
        Simulator::Stop();
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << Simulator::Now().GetMilliSeconds() << " " << seq  << " " << responseTime.GetMilliSeconds() << std::endl;

    // Close the file
    file.close();
}



void
Consumer::AggregateTimeRecorder(Time aggregateTime, uint32_t seq) {
    // Open the file using fstream in append mode
    std::ofstream file(aggregateTime_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << aggregateTime_recorder << std::endl;
        return;
    }

    // Write aggregation time to file, followed by a new line
    file << Simulator::Now().GetMilliSeconds() << " " << seq << " " << aggregateTime.GetMilliSeconds() << std::endl;

    file.close();
}



void
Consumer::InitializeLogFile()
{
    // Check whether object path exists, create it if not
    CheckDirectoryExist(conFolderPath);
    CheckDirectoryExist(fwdFolderPath);

    // Open the file and clear all contents for all log files
    for (int roundIndex = 0; roundIndex < globalTreeRound.size(); roundIndex++) {
        for (int i = 0; i < globalTreeRound[roundIndex].size(); i++) {
            // RTT/RTO recorder
            responseTime_recorder[globalTreeRound[roundIndex][i]] = conFolderPath + m_prefix.toUri() + "_RTT_" + globalTreeRound[roundIndex][i] + ".txt";
            RTO_recorder[globalTreeRound[roundIndex][i]] = conFolderPath + m_prefix.toUri() + "_RTO_" + globalTreeRound[roundIndex][i] + ".txt";
            OpenFile(responseTime_recorder[globalTreeRound[roundIndex][i]]);
            OpenFile(RTO_recorder[globalTreeRound[roundIndex][i]]);
        }
    }

    for (const auto& round : globalTreeRound) {
        for (const auto& prefix : round) {
            qsNew_recorder[prefix] = conFolderPath + m_prefix.toUri() + "_queue_" + prefix + ".txt";
            inFlight_recorder[prefix] = conFolderPath + m_prefix.toUri() + "_inFlight_" + prefix + ".txt";
            OpenFile(qsNew_recorder[prefix]);
            OpenFile(inFlight_recorder[prefix]);
        }
    }

    // Aggregation time, AggTree, throughput
    aggregateTime_recorder = conFolderPath + m_prefix.toUri() + "_aggregationTime.txt";
    OpenFile(aggregateTime_recorder);
    OpenFile(throughput_recorder);
    OpenFile(aggTree_recorder);

    // Result log
    OpenFile(result_recorder);
}



void
Consumer::InitializeParameter()
{
    int i = 0;
    // Each round
    for (const auto& round : globalTreeRound) {
        // Individual flow
        for (const auto& prefix : round) {
            //* Initialize RTO and RTT parameters
            initRTO[prefix] = false;
            RTO_threshold[prefix] = 5 * m_retxTimer;

            //RTT_threshold[prefix] = 0;
            RTT_count[prefix] = 0;
            RTT_historical_estimation[prefix] = 0;

            //* Initialize sequence map, interest queue
            SeqMap[prefix] = 0;
            interestQueue[prefix] = std::deque<uint32_t>();
            m_inFlight[prefix] = 0;

            // Initialize QueueSize-based CC info
            m_qsSlidingWindows[prefix] = utils::SlidingWindow<double>(MilliSeconds(m_qsTimeDuration));
            m_estimatedBW[prefix] = 0;
            m_rateLimit[prefix] = m_qsInitRate;
            firstData[prefix] = true;
            RTT_estimation_qs[prefix] = 0;
            nackSignal[prefix] = false;
            timeoutSignal[prefix] = false;
            m_lastBW[prefix] = 0;
            m_ccState[prefix] = "Startup";
            m_inflightLimit[prefix] = 0;
        }
    i++;
    }

    // Init params for interest sending rate pacing
    isRTTEstimated = false;
}



bool
Consumer::CanDecreaseWindow(std::string prefix, int64_t threshold)
{
    if (Simulator::Now().GetMilliSeconds() - lastWindowDecreaseTime[prefix].GetMilliSeconds() > threshold) {
        return true;
    } else {
        return false;
    }
}



void
Consumer::InFlightRecorder(std::string prefix)
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(inFlight_recorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << inFlight_recorder[prefix] << std::endl;
        return;
    }

    file << Simulator::Now().GetMilliSeconds() << " " << m_inFlight[prefix] << std::endl;

    file.close();
}



void
Consumer::ThroughputRecorder(int interestThroughput, int dataThroughput, Time start_simulation, Time start_throughput)
{
    // Open the file using fstream in append mode
    std::ofstream file(throughput_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << throughput_recorder << std::endl;
        return;
    }

    // Write aggregation time to file, followed by a new line
    file << interestThroughput << " " << dataThroughput << " " << linkCount << " " << start_throughput.GetMilliSeconds() << " " << Simulator::Now().GetMilliSeconds() << std::endl;

    file.close();
}



void
Consumer::AggTreeRecorder() {

    // Open the file using fstream in append mode
    std::ofstream file(aggTree_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << aggTree_recorder << std::endl;
        return;
    }

    int i = 0;
    NS_LOG_DEBUG("Start writing aggregation tree into the log file.");
    for (const auto& map : aggregationTree) {
        for (const auto& pair : map) {
            // Aggregator
            if (pair.first != m_nodeprefix) {
                file << pair.first << ": ";
                for (const auto& value : pair.second) {
                    file << value << " ";
                }
            }
            // Consumer
            else {
                file << pair.first << " -> round " << i << ": ";
                for (const auto& value : pair.second) {
                    file << value << " ";
                }
            }
            file << std::endl;
        }
        i++;
    }
    file.close();
}



void
Consumer::ResultRecorder(uint32_t iteNum, int timeoutNum, int64_t aveAggTime, int64_t totalTime)
{
    // Open the file using fstream in append mode
    std::ofstream file(result_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << result_recorder << std::endl;
        return;
    }

    file << "Consumer's result" << std::endl;
    file << "Total iterations: " << iteNum << std::endl;
    file << "Timeout is triggered for " << timeoutNum<< " times" << std::endl;
    file << "Data queue overflow is triggered for " << dataOverflow << " times" << std::endl;
    file << "Nack(upstream interest queue overflow) is triggered for " << nackCount << " times" << std::endl;
    file << "Average aggregation time: " << aveAggTime << " ms." << std::endl;
    file << "Total aggregation time: " << totalTime / 1000 << " ms." << std::endl;
    file << "-----------------------------------" << std::endl;
}



void
Consumer::QueueRecorder(std::string prefix, double queueSize)
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
Consumer::RTTMeasure(std::string prefix, int64_t resTime)
{
    // Update RTT estimation
    if (RTT_estimation_qs[prefix] == 0) {
        RTT_estimation_qs[prefix] = resTime;
    } else {
        RTT_estimation_qs[prefix] = m_EWMAFactor * RTT_estimation_qs[prefix] + (1 - m_EWMAFactor) * resTime;
    }
}



double
Consumer::GetDataRate(std::string prefix)
{
    double rawDataRate = m_qsSlidingWindows[prefix].GetDataArrivalRate();

    // "0": sliding window size is less than one, keep init rate as data arrival rate; "-1" indicates error
    if (rawDataRate == -1) {
        NS_LOG_INFO("Returned data arrival rate is -1, please check!");
        Simulator::Stop();
        return 0;  
    } else if (rawDataRate == 0) {
        NS_LOG_INFO("Sliding window is not enough, use 0 as data arrival rate: " << " 0 pkgs/ms");
        return 0.0;
    } else {
        return rawDataRate;
    }
}



void
Consumer::BandwidthEstimation(std::string prefix)
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
                ", Arrival Rate: " << dataArrivalRate * 1000 << 
                " pkgs/ms, Bandwidth estimation: " << m_estimatedBW[prefix] * 1000 << " pkgs/ms");
}



void
Consumer::RateLimitUpdate(std::string prefix)
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
    m_rateEvent[prefix] = Simulator::Schedule(MicroSeconds(RTT_estimation_qs[prefix]), &Consumer::RateLimitUpdate, this, prefix);
}





} // namespace ndn
} // namespace ns3
