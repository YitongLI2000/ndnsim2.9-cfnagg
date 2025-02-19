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

#include "ndn-producer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

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

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "ModelData.hpp"

#include <random>
#include <vector>
#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.Producer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Producer);

TypeId
Producer::GetTypeId(void)
{
    static TypeId tid =
    TypeId("ns3::ndn::Producer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<Producer>()
      .AddAttribute("Prefix", 
                    "Prefix, for which producer has the data", 
                    StringValue("/"),
                    MakeNameAccessor(&Producer::m_prefix), 
                    MakeNameChecker())
      .AddAttribute("PrefixNum", 
                    "Prefix number", 
                    IntegerValue(),
                    MakeIntegerAccessor(&Producer::m_prefixnum), 
                    MakeIntegerChecker<int32_t>())
      .AddAttribute("Postfix",
                    "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                    StringValue("/"), 
                    MakeNameAccessor(&Producer::m_postfix), 
                    MakeNameChecker())
      .AddAttribute("PayloadSize", 
                    "Virtual payload size for Content packets", 
                    UintegerValue(1024),
                    MakeUintegerAccessor(&Producer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", 
                    "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), 
                    MakeTimeAccessor(&Producer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute("Signature",
                    "Fake signature, 0 valid signature (default), other values application-specific",
                    UintegerValue(0), 
                    MakeUintegerAccessor(&Producer::m_signature),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), 
                    MakeNameAccessor(&Producer::m_keyLocator), 
                    MakeNameChecker())
      .AddAttribute("DataSize",
                    "Define the data content size",
                    IntegerValue(150),
                    MakeIntegerAccessor(&Producer::m_dataSize),
                    MakeIntegerChecker<int>())                    ;
    return tid;
}

Producer::Producer()
{
  //NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
Producer::StartApplication()
{
  //NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
Producer::StopApplication()
{
  //NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
Producer::OnInterest(shared_ptr<const Interest> interest)
{
    App::OnInterest(interest); // tracing inside

    if (!m_active)
    return;

    Name dataName(interest->getName());
    // dataName.append(m_postfix);
    // dataName.appendVersion();

    auto data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

    // generate new data content
    // new data format and generate random fix size of model parameters
    ModelData modelData;

    std::default_random_engine generator(std::random_device{}()); // create random generator
    std::uniform_real_distribution<double> distribution(0.0f, 10.0f); // define range (0.0, 10.0)
    modelData.parameters.clear(); // clear the previous result
    for (int i = 0; i < m_dataSize; ++i){
        modelData.parameters.push_back(distribution(generator)); // generate random double range (0.0, 10.0)
    }

    std::vector<uint8_t> buffer;
    serializeModelData(modelData, buffer); // serialize data packet
    data->setContent(make_shared< ::ndn::Buffer>(buffer.begin(), buffer.end()));

    // end of data content



    SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

    if (m_keyLocator.size() > 0) {
        signatureInfo.setKeyLocator(m_keyLocator);
    }

    data->setSignatureInfo(signatureInfo);

    ::ndn::EncodingEstimator estimator;
    ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
    encoder.appendVarNumber(m_signature);
    data->setSignatureValue(encoder.getBuffer());

    NS_LOG_INFO(m_prefix << " -> node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
    NS_LOG_INFO("The returned data packet size is: " << data->wireEncode().size());

    // to create real wire encoding
    data->wireEncode();

    m_transmittedDatas(data, this, m_face);
    m_appLink->onReceiveData(*data);
}

} // namespace ndn
} // namespace ns3
