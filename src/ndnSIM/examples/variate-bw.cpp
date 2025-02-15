#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/error-model.h"
#include "ns3/point-to-point-module.h"


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h> // Include the header for mkdir function

namespace ns3 {

    struct ConfigParams {
        std::string Topology;
        int RTTWindowSize;
        int Constraint;
        std::string Window;
        int InitPace;
        std::string CcAlgorithm;
        double Alpha;
        double Beta;
        double Gamma;
        double EWMAFactor;
        double ThresholdFactor;
        bool UseCwa;
        bool UseWIS;
        bool UseCubicFastConv;
        int ConInterestQueue;
        int ConDataQueue;
        int AggInterestQueue;
        int AggDataQueue;
        int Iteration;
        int DataSize;
        int QueueThreshold;
        int InFlightThreshold;
        double QSMDFactor;
        double QSRPFactor;
        int QSSlidingWindowDuration;
        double QSInitRate;
    };

    /**
     * Get all required config parameters
     * @return
     */
    ConfigParams GetConfigParams() {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini("src/ndnSIM/experiments/simulation_settings/config.ini", pt);

        ConfigParams params;
        params.Topology = pt.get<std::string>("General.TopologyType");
        params.Constraint = pt.get<int>("General.Constraint");
        params.Window = pt.get<std::string>("General.Window");
        params.CcAlgorithm = pt.get<std::string>("General.CcAlgorithm");
        params.Alpha = pt.get<double>("General.Alpha");
        params.Beta = pt.get<double>("General.Beta");
        params.Gamma = pt.get<double>("General.Gamma");
        params.EWMAFactor = pt.get<double>("General.EWMAFactor");
        params.ThresholdFactor = pt.get<double>("General.ThresholdFactor");
        params.UseCwa = pt.get<bool>("General.UseCwa");
        params.UseWIS = pt.get<bool>("General.UseWIS");
        params.RTTWindowSize = pt.get<int>("General.RTTWindowSize");
        params.DataSize = pt.get<int>("General.DataSize");
        params.ConInterestQueue = pt.get<int>("Consumer.ConInterestQueue");
        params.ConDataQueue = pt.get<int>("Consumer.ConDataQueue");
        params.AggInterestQueue = pt.get<int>("Aggregator.AggInterestQueue");
        params.AggDataQueue = pt.get<int>("Aggregator.AggDataQueue");
        params.Iteration = pt.get<int>("Consumer.Iteration");
        params.UseCubicFastConv = pt.get<bool>("General.UseCubicFastConv");
        params.InitPace = pt.get<int>("General.InitPace");
        params.QueueThreshold = pt.get<int>("QS.QueueThreshold");
        params.QSMDFactor = pt.get<double>("QS.MDFactor");
        params.QSRPFactor = pt.get<double>("QS.RPFactor");
        params.QSSlidingWindowDuration = pt.get<int>("QS.SlidingWindow");
        params.QSInitRate = pt.get<double>("QS.InitRate");
        params.InFlightThreshold = pt.get<int>("QS.InFlightThreshold");

        return params;
    }


    /**
     * Get constraints from config.ini
     * @return
     */
    int GetConstraint() {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini("src/ndnSIM/experiments/simulation_settings/config.ini", pt);

        int constraint = pt.get<int>("General.Constraint");
        return constraint;
    }

    /**
     * Define packet loss tracing function
     * @param context
     * @param packet
     */
    void PacketDropCallback(std::string context, Ptr<const Packet> packet){
        uint32_t droppedPacket = 0;
        droppedPacket++;
        std::cout << "Packet dropped! Total dropped packets: " << droppedPacket << std::endl;
    }



    /**
     * Create a directory
     * @param path
     */
    void CreateDirectory(const std::string& path) {
        // Create a directory with user read/write/execute permissions
        if (mkdir(path.c_str(), S_IRWXU) == -1) {
            if (errno == EEXIST) {
                std::cout << "Directory " << path << " already exists." << std::endl;
            } else {
                // Check for other errors
                std::cerr << "Error creating directory " << std::endl;
            }
        } else {
            std::cout << "Directory created: " << path << std::endl;
        }
    }



    /**
     * Dynamically change bandwidth of all access layer links (producers/consumers to forwarders).
     * Access layer links are identified based on node names (producers: "pro*", consumers: "con*").
     */
    void ChangeAccessLayerBandwidth(std::string newBandwidth) {
        for (NodeContainer::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
            Ptr<Node> node = *i;
            std::string nodeName = Names::FindName(node);

            // Check if the node is a producer or consumer (access layer)
            if (nodeName.find("pro") == 0 || nodeName.find("con") == 0) {
                // Iterate through all devices of this node
                for (uint32_t j = 0; j < node->GetNDevices(); ++j) {
                    Ptr<NetDevice> device = node->GetDevice(j);
                    Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(device);

                    if (p2pDevice) {
                        // Change the DataRate on the PointToPointNetDevice
                        p2pDevice->SetAttribute("DataRate", DataRateValue(DataRate(newBandwidth)));

                        // Log the bandwidth change
                        std::cout << "Changed bandwidth of link connected to node: " << nodeName
                                << " to " << newBandwidth << std::endl;
                    }
                }
            }
        }
    }



    int main(int argc, char* argv[])
    {
        // Get constraint from config.ini
        ConfigParams params = GetConfigParams();

        CommandLine cmd;
        cmd.Parse(argc, argv);

        PointToPointHelper p2p;

        AnnotatedTopologyReader topologyReader("", 25);
        // Choose corresponding network topology
        if (params.Topology == "DCN") {
            topologyReader.SetFileName("src/ndnSIM/examples/topologies/DataCenterTopology.txt");
        } else if (params.Topology == "ISP") {
            topologyReader.SetFileName("src/ndnSIM/examples/topologies/ISPTopology.txt");
        } else {
            std::cerr << "Topology type error, please check!" << std::endl;
            return 1;
        }

        topologyReader.Read();

        // Create error model to add packet loss
        Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
        em->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
        em->SetAttribute("ErrorRate", DoubleValue(0.001));

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.InstallAll();

        ndn::GlobalRoutingHelper GlobalRoutingHelper;

        // Set BestRoute strategy
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

        // Add packet drop tracing to all nodes
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDrop", MakeCallback(&PacketDropCallback));



        for (NodeContainer::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
            Ptr<Node> node = *i;
            std::string nodeName = Names::FindName(node);

            if (nodeName.find("con") == 0) {
                // Config consumer's attribute on ConsumerINA class
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerINA");
                consumerHelper.SetPrefix("/" + nodeName);
                consumerHelper.SetAttribute("TopologyType", StringValue(params.Topology));
                consumerHelper.SetAttribute("Iteration", IntegerValue(params.Iteration));
                consumerHelper.SetAttribute("UseCwa", BooleanValue(params.UseCwa));
                consumerHelper.SetAttribute("NodePrefix", StringValue("con0"));
                consumerHelper.SetAttribute("Constraint", IntegerValue(params.Constraint));
                consumerHelper.SetAttribute("Window", StringValue(params.Window));
                consumerHelper.SetAttribute("Alpha", DoubleValue(params.Alpha));
                consumerHelper.SetAttribute("Beta", DoubleValue(params.Beta));
                consumerHelper.SetAttribute("Gamma", DoubleValue(params.Gamma));
                consumerHelper.SetAttribute("EWMAFactor", DoubleValue(params.EWMAFactor));
                consumerHelper.SetAttribute("ThresholdFactor", DoubleValue(params.ThresholdFactor));
                consumerHelper.SetAttribute("InterestQueueSize", IntegerValue(params.ConInterestQueue));
                consumerHelper.SetAttribute("DataQueueSize", IntegerValue(params.ConDataQueue));
                consumerHelper.SetAttribute("RTTWindowSize", IntegerValue(params.RTTWindowSize));
                consumerHelper.SetAttribute("UseWIS", BooleanValue(params.UseWIS));
                consumerHelper.SetAttribute("DataSize", IntegerValue(params.DataSize));
                consumerHelper.SetAttribute("CcAlgorithm", StringValue(params.CcAlgorithm));
                consumerHelper.SetAttribute("UseCubicFastConv", BooleanValue(params.UseCubicFastConv));
                consumerHelper.SetAttribute("InitPace", IntegerValue(params.InitPace));
                consumerHelper.SetAttribute("QueueThreshold", IntegerValue(params.QueueThreshold));
                consumerHelper.SetAttribute("QSMDFactor", DoubleValue(params.QSMDFactor));
                consumerHelper.SetAttribute("QSRPFactor", DoubleValue(params.QSRPFactor));
                consumerHelper.SetAttribute("QSSlidingWindowDuration", IntegerValue(params.QSSlidingWindowDuration));
                consumerHelper.SetAttribute("QSInitRate", DoubleValue(params.QSInitRate));
                consumerHelper.SetAttribute("InFlightThreshold", IntegerValue(params.InFlightThreshold));

                // Add consumer prefix in all nodes' routing info
                auto app1 = consumerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                app1.Start(Seconds(1));
            } else if (nodeName.find("agg") == 0) {
                // Config aggregator's attribute on aggregator class
                ndn::AppHelper aggregatorHelper("ns3::ndn::Aggregator");
                aggregatorHelper.SetPrefix("/" + nodeName);
                aggregatorHelper.SetAttribute("Iteration", IntegerValue(params.Iteration));
                aggregatorHelper.SetAttribute("UseCwa", BooleanValue(params.UseCwa));
                aggregatorHelper.SetAttribute("Window", StringValue(params.Window));
                aggregatorHelper.SetAttribute("Alpha", DoubleValue(params.Alpha));
                aggregatorHelper.SetAttribute("Beta", DoubleValue(params.Beta));
                aggregatorHelper.SetAttribute("Gamma", DoubleValue(params.Gamma));
                aggregatorHelper.SetAttribute("EWMAFactor", DoubleValue(params.EWMAFactor));
                aggregatorHelper.SetAttribute("ThresholdFactor", DoubleValue(params.ThresholdFactor));
                aggregatorHelper.SetAttribute("InterestQueueSize", IntegerValue(params.AggInterestQueue));
                aggregatorHelper.SetAttribute("DataQueueSize", IntegerValue(params.AggDataQueue));
                aggregatorHelper.SetAttribute("RTTWindowSize", IntegerValue(params.RTTWindowSize));
                aggregatorHelper.SetAttribute("UseWIS", BooleanValue(params.UseWIS));
                aggregatorHelper.SetAttribute("DataSize", IntegerValue(params.DataSize));
                aggregatorHelper.SetAttribute("CcAlgorithm", StringValue(params.CcAlgorithm));
                aggregatorHelper.SetAttribute("UseCubicFastConv", BooleanValue(params.UseCubicFastConv));
                aggregatorHelper.SetAttribute("InitPace", IntegerValue(params.InitPace));
                aggregatorHelper.SetAttribute("QueueThreshold", IntegerValue(params.QueueThreshold));
                aggregatorHelper.SetAttribute("QSMDFactor", DoubleValue(params.QSMDFactor));
                aggregatorHelper.SetAttribute("QSRPFactor", DoubleValue(params.QSRPFactor));
                aggregatorHelper.SetAttribute("QSSlidingWindowDuration", IntegerValue(params.QSSlidingWindowDuration));
                aggregatorHelper.SetAttribute("QSInitRate", DoubleValue(params.QSInitRate));
                aggregatorHelper.SetAttribute("InFlightThreshold", IntegerValue(params.InFlightThreshold));

                // Add aggregator prefix in all nodes' routing info
                auto app2 = aggregatorHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                GlobalRoutingHelper.AddOrigins("/" + nodeName, node);
                app2.Start(Seconds(0));
            } else if (nodeName.find("pro") == 0) {
                // Install Producer on producer nodes
                ndn::AppHelper producerHelper("ns3::ndn::Producer");
                producerHelper.SetPrefix("/" + nodeName);
                producerHelper.SetAttribute("DataSize", IntegerValue(params.DataSize));

                // Add producer prefix in all nodes' routing info
                producerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                GlobalRoutingHelper.AddOrigins("/" + nodeName, node);

                // Add error rate to producer
                //Ptr<NetDevice> proDevice = node->GetDevice(0);
                //proDevice->SetAttribute("ReceiveErrorModel", PointerValue(em));
            }
        }
        // Calculate and install FIBs
        ndn::GlobalRoutingHelper::CalculateRoutes();

        // Schedule bandwidth change after 3 seconds
        Simulator::Schedule(Seconds(3), &ChangeAccessLayerBandwidth, "12Mbps"); // Change bandwidth to 12Mbps

        // Capture packets
/*         std::string packetTraceDir = "src/ndnSIM/results/packet_traces/";
        CreateDirectory(packetTraceDir);
        PointToPointHelper pointToPoint;
        pointToPoint.EnablePcapAll(packetTraceDir); */

        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
