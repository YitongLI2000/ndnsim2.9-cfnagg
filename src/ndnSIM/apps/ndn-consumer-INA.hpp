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
 **/

#ifndef NDN_CONSUMER_INA_H
#define NDN_CONSUMER_INA_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-consumer.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include <limits>

namespace ns3 {
namespace ndn {


/**
 * @ingroup ndn-apps
 * \brief NDN consumer application with more advanced congestion control options
 *
 * This app uses the algorithms from "A Practical Congestion Control Scheme for Named
 * Data Networking" (https://dl.acm.org/citation.cfm?id=2984369).
 *
 * It implements slow start, conservative window adaptation (RFC 6675),
 * and 3 different TCP algorithms: AIMD, BIC, and CUBIC (RFC 8312).
 */
class ConsumerINA : public Consumer {
public:

    /**
     * Initiate attributes for consumer class, some of them may be used, some are optional
     *
     * @return Total TypeId
     */
    static TypeId
    GetTypeId();

    /**
     * Constructor
     */
    ConsumerINA();


    /**
     * Override from consumer
     */
    virtual void
    StartApplication();


    /**
     * Override from consumer
     * @param data
     */
    virtual void
    OnData(shared_ptr<const Data> data) override;


    /**
     * Invoked when Nack is triggered
     * @param nack
     */
    virtual void
    OnNack(shared_ptr<const lp::Nack> nack) override;


    /**
     * Multiplicative decrease cwnd when timeout
     * @param nameString
     */
    virtual void
    OnTimeout(std::string nameString) override;


    /**
     * Override from consumer class
     * @param newName
     */
    virtual void
    SendInterest(shared_ptr<Name> newName);


    /**
     * Based on cwnd, schedule whether sends packet or not
     * Currently is called by each flow individually
     * Note that it's only activated after initialization, initialization process is controlled individually
     * @param prefix
     */
    virtual void
    ScheduleNextPacket(std::string prefix);

private:

    /**
     * Increase cwnd
     */
    void
    WindowIncrease(std::string prefix);


    /**
     * Decrease cwnd
     * @param type
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
     * Set cwnd
     * @param window
     */
    virtual void
    SetWindow(uint32_t window);


    /**
     * Return cwnd
     * @return cwnd
     */
    uint32_t
    GetWindow() const;


    /**
     * Record window size into file
     */
    void WindowRecorder(std::string prefix);


    /**
     * Initialize log file for "consumer_window.txt"
     */
    void InitializeLogFile();


    /**
     * Initialize parameters
     */
    void InitializeParameter();

public:
    typedef std::function<void(double)> WindowTraceCallback;

private:
    bool m_setInitialWindowOnTimeout; // Seems not enabled

    EventId windowMonitor;

};

} // namespace ndn
} // namespace ns3

#endif // NDN_CONSUMER_INA_H
