/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018  Regents of the University of California.
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
 * This class is written by Yitong, which inherits "ndn-consumer" class, aiming to provide cwnd management and congestion function
 * This class is the one being instantiated in ndnSIM's scenario file
 **/

#include "ndn-consumer-INA.hpp"
#include <fstream>
#include <string>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerINA");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerINA);


TypeId
ConsumerINA::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerINA")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<ConsumerINA>()

      .AddAttribute("Alpha",
                    "TCP Multiplicative Decrease factor",
                    DoubleValue(0.5),
                    MakeDoubleAccessor(&ConsumerINA::m_alpha),
                    MakeDoubleChecker<double>())
      .AddAttribute("Beta",
                    "Local congestion decrease factor",
                    DoubleValue(0.6),
                    MakeDoubleAccessor(&ConsumerINA::m_beta),
                    MakeDoubleChecker<double>())
      .AddAttribute("Gamma",
                    "Remote congestion decrease factor",
                    DoubleValue(0.7),
                    MakeDoubleAccessor(&ConsumerINA::m_gamma),
                    MakeDoubleChecker<double>())
      .AddAttribute("AddRttSuppress",
                    "Minimum number of RTTs (1 + this factor) between window decreases",
                    DoubleValue(0.5), // This default value was chosen after manual testing
                    MakeDoubleAccessor(&ConsumerINA::m_addRttSuppress),
                    MakeDoubleChecker<double>())
      .AddAttribute("ReactToCongestionMarks",
                    "If true, process received congestion marks",
                    BooleanValue(true),
                    MakeBooleanAccessor(&ConsumerINA::m_reactToCongestionMarks),
                    MakeBooleanChecker())
      .AddAttribute("UseCwa",
                    "If true, use Conservative Window decrease Algorithm",
                    BooleanValue(false),
                    MakeBooleanAccessor(&ConsumerINA::m_useCwa),
                    MakeBooleanChecker())
      .AddAttribute("Window", "Initial size of the window", StringValue("1"),
                    MakeUintegerAccessor(&ConsumerINA::GetWindow, &ConsumerINA::SetWindow),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("InitialWindowOnTimeout", "Set window to initial value when timeout occurs",
                    BooleanValue(true),
                    MakeBooleanAccessor(&ConsumerINA::m_setInitialWindowOnTimeout),
                    MakeBooleanChecker());

  return tid;
}



ConsumerINA::ConsumerINA()
{

}



void
ConsumerINA::SendInterest(shared_ptr<Name> newName)
{
/*     // Get the prefix of the interest, which is the flow name
    std::string flow = newName->get(0).toUri();

    // Record inFlight for congestion control
    m_inFlight[flow]++; */

    Consumer::SendInterest(newName);
}



void
ConsumerINA::ScheduleNextPacket(std::string prefix)
{
    if (interestQueue.find(prefix) == interestQueue.end()) {
        NS_LOG_DEBUG("Flow " << prefix << " is not found in the interest queue.");
        Simulator::Stop();
        return;
    }

    //? Check whether interest queue is null, if so, split new interests...
    // Interest splitting
    if (interestQueue[prefix].empty()) {
        // Reach the last iteration, stop scheduling new packets for current flow
        if (globalSeq == m_iteNum) {
            NS_LOG_INFO("All iterations have been finished, no need to schedule new interests.");
            return;
        }

        // Check whether interest queue is full
        if (!InterestSplitting()) {
            //? Fail to split new interests, schedule this flow later
            NS_LOG_DEBUG("Other flows' queue is full, schedule this flow later.");
        } else {
            if (m_sendEvent[prefix].IsRunning()) {
                Simulator::Remove(m_sendEvent[prefix]);
                NS_LOG_DEBUG("Suspicious, remove the previous event.");
            } 
            
            m_sendEvent[prefix] = Simulator::ScheduleNow(&Consumer::SendPacket, this, prefix);
        }
    } else {
        if (m_sendEvent[prefix].IsRunning()) {
            Simulator::Remove(m_sendEvent[prefix]);
            NS_LOG_DEBUG("Suspicious, remove the previous event.");
        } 
            
        m_sendEvent[prefix] = Simulator::ScheduleNow(&Consumer::SendPacket, this, prefix);
    }

    // Schdule next scheduling event
    double nextTime = 1/m_rateLimit[prefix]; // Unit: us
    NS_LOG_INFO("Flow " << prefix << " -> Schedule next sending event after " << nextTime / 1000 << " ms.");
    m_scheduleEvent[prefix] = Simulator::Schedule(MicroSeconds(nextTime), &ConsumerINA::ScheduleNextPacket, this, prefix);
}



void
ConsumerINA::StartApplication()
{
    Consumer::StartApplication();
}



void
ConsumerINA::OnNack(shared_ptr<const lp::Nack> nack)
{
    Consumer::OnNack(nack);
}



void
ConsumerINA::OnData(shared_ptr<const Data> data)
{
    Consumer::OnData(data);
}



void
ConsumerINA::OnTimeout(std::string nameString)
{
    Consumer::OnTimeout(nameString);
}



void
ConsumerINA::SetWindow(uint32_t window)
{
    m_initialWindow = window;
}



uint32_t
ConsumerINA::GetWindow() const
{
    return m_initialWindow;
}



void
ConsumerINA::WindowIncrease(std::string prefix)
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
ConsumerINA::WindowDecrease(std::string prefix, std::string type)
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
        else if (type == "ConsumerCongestion") {
            m_ssthresh[prefix] = m_window[prefix] * m_beta;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else if (type == "RemoteCongestion") {
            m_ssthresh[prefix] = m_window[prefix] * m_gamma;
            m_window[prefix] = m_ssthresh[prefix];
        }
        else {
            NS_LOG_INFO("Unknown congestion type, please check!");
            Simulator::Stop();
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
        else if (type == "ConsumerCongestion") {
            CubicDecrease(prefix, type);
        }
        else if (type == "RemoteCongestion") {
            // Do nothing, currently disabled
        }
        else {
            NS_LOG_INFO("Unknown congestion type, please check!");
            Simulator::Stop();
        }
    } else {
        NS_LOG_DEBUG("CC alogrithm can't be recognized, please check!");
        Simulator::Stop();
    }


    // Window size can't be reduced below 1
    if (m_window[prefix] < m_minWindow) {
        m_window[prefix] = m_minWindow;
    }

    NS_LOG_DEBUG("Flow: " << prefix << ". Window size decreased to " << m_window[prefix] << ". Reason: " << type);
}



void
ConsumerINA::CubicIncerase(std::string prefix) 
{
    // 1. Time since last congestion event in Seconds
    const double t = (Simulator::Now().GetMicroSeconds() - lastWindowDecreaseTime[prefix].GetMicroSeconds()) / 1e6;
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
ConsumerINA::CubicDecrease(std::string prefix, std::string type)
{
    // Traditional cubic window decrease
    m_cubicWmax[prefix] = m_window[prefix];
    m_ssthresh[prefix] = m_window[prefix] * m_cubicBeta;
    m_ssthresh[prefix] = std::max<double>(m_ssthresh[prefix], m_minWindow);
    m_window[prefix] = m_window[prefix] * m_cubicBeta;

    // Cubic with fast convergence
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
ConsumerINA::WindowRecorder(std::string prefix)
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(windowRecorder[prefix], std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << windowRecorder[prefix] << std::endl;
        return;
    }

    file << ns3::Simulator::Now().GetMicroSeconds() << " " << m_window[prefix] << " " << m_ssthresh[prefix] << " " << interestQueue[prefix].size() << std::endl;

    file.close();
}



void
ConsumerINA::InitializeLogFile()
{
    //Consumer::InitializeLogFile();
}



void
ConsumerINA::InitializeParameter()
{
    //Consumer::InitializeParameter();

/*     // Initialize cwnd
    for (const auto& round : globalTreeRound) {
        for (const auto& flow : round) {
            m_window[flow] = m_initialWindow;
            m_inFlight[flow] = 0;
            m_ssthresh[flow] = std::numeric_limits<double>::max();

            // Initialize CUBIC factor
            m_cubicLastWmax[flow] = m_initialWindow;
            m_cubicWmax[flow] = m_initialWindow;
            lastWindowDecreaseTime[flow] = Simulator::Now();
        }

    } */
}
} // namespace ndn
} // namespace ns3
