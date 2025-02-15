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

#include "ndn-app.hpp"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"

#include "model/ndn-l3-protocol.hpp"
#include "model/ndn-app-link-service.hpp"
#include "model/null-transport.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>


//#include "aggregationTree.hpp"
#include "src/ndnSIM/apps/algorithm/include/AggregationTree.hpp"
#include "src/ndnSIM/apps/algorithm/utility/utility.hpp"


NS_LOG_COMPONENT_DEFINE("ndn.App");

namespace fs = std::filesystem;
namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(App);

TypeId
App::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ndn::App")
                        .SetGroupName("Ndn")
                        .SetParent<Application>()
                        .AddConstructor<App>()

                        .AddTraceSource("ReceivedInterests", "ReceivedInterests",
                                        MakeTraceSourceAccessor(&App::m_receivedInterests),
                                        "ns3::ndn::App::InterestTraceCallback")

                        .AddTraceSource("ReceivedDatas", "ReceivedDatas",
                                        MakeTraceSourceAccessor(&App::m_receivedDatas),
                                        "ns3::ndn::App::DataTraceCallback")

                        .AddTraceSource("ReceivedNacks", "ReceivedNacks",
                                        MakeTraceSourceAccessor(&App::m_receivedNacks),
                                        "ns3::ndn::App::NackTraceCallback")

                        .AddTraceSource("TransmittedInterests", "TransmittedInterests",
                                        MakeTraceSourceAccessor(&App::m_transmittedInterests),
                                        "ns3::ndn::App::InterestTraceCallback")

                        .AddTraceSource("TransmittedDatas", "TransmittedDatas",
                                        MakeTraceSourceAccessor(&App::m_transmittedDatas),
                                        "ns3::ndn::App::DataTraceCallback")

                        .AddTraceSource("TransmittedNacks", "TransmittedNacks",
                                        MakeTraceSourceAccessor(&App::m_transmittedNacks),
                                        "ns3::ndn::App::NackTraceCallback");
  return tid;
}

App::App()
  : m_active(false)
  , m_face(0)
  , m_appId(std::numeric_limits<uint32_t>::max())
{
}

App::~App()
{
}


/**
 * Actual function is implemented in consumer
 */
void
App::ConstructAggregationTree() {

}



/**
 * Return all child nodes for given map and parent node, not called for now
 * @param key parent node
 * @param treeMap given mapping
 * @return
 */
std::set<std::string>
App::findLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap)
{
    std::set<std::string> result;
    auto it = treeMap.find(key);
    if (it != treeMap.end()) {
        for (const auto& subkey : it->second) {
            if (treeMap.find(subkey) != treeMap.end()) {
                auto subResult = findLeafNodes(subkey, treeMap);
                result.insert(subResult.begin(), subResult.end());
            } else {
                result.insert(subkey);
            }
        }
    }
    return result;
}



/**
 * Return a mapping (key: child node, value: leaf nodes connected at the upstream tier - producers)
 * E.g. {agg0: [pro0, pro1]}
 * @param key parent node
 * @param treeMap input mapping
 * @return
 */
std::map<std::string, std::set<std::string>>
App::getLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap)
{
    std::map<std::string, std::set<std::string>> result;
    auto it = treeMap.find(key);
    if (it != treeMap.end()) {
        for (const auto& subkey : it->second) {
            if (treeMap.find(subkey) != treeMap.end()) {
                result[subkey] = findLeafNodes(subkey, treeMap);
            } else {
                result[subkey].insert(subkey);
            }
        }
    }
    return result;
}



/**
 * Return round index
 * @param roundVec vector consists all related nodes (aggregator) in one iteration
 * @param target target node
 * @return
 */
int
App::findRoundIndex(const std::vector<std::vector<std::string>>& roundVec, const std::string& target)
{
    for (int i = 0; i < roundVec.size(); ++i) {
        for (int j = 0; j < roundVec[i].size(); ++j) {
            if (roundVec[i][j] == target) {
                return i; // Return index
            }
        }
    }
    std::cout << "Error! Can't find round index" << std::endl;
    return -1;
}



/**
 * Check whether this folder exists, if not, create it
 * @param path
 */
void
App::CheckDirectoryExist(const std::string& path)
{
    if (!fs::exists(path)) {
        if (!fs::create_directories(path)) {
            std::cerr << "Failed to create directory: " << path << std::endl;
            exit(EXIT_FAILURE); // Stop execution if unable to create directory
        }
    }
}



/**
 * Open and clear the file
 * @param filename
 */
void
App::OpenFile(const std::string& filename)
{
    std::ofstream file(filename, std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << filename << std::endl;
    }
    // ToDo: for testing, delete later
    else {
        //std::cout << "Open " << filename << " successfully!" << std::endl;
    }
    file.close();
}



void
App::DoInitialize()
{
  NS_LOG_FUNCTION_NOARGS();

  // find out what is application id on the node
  for (uint32_t id = 0; id < GetNode()->GetNApplications(); ++id) {
    if (GetNode()->GetApplication(id) == this) {
      m_appId = id;
    }
  }

  Application::DoInitialize();
}

void
App::DoDispose(void)
{
  NS_LOG_FUNCTION_NOARGS();

  // Unfortunately, this causes SEGFAULT
  // The best reason I see is that apps are freed after ndn stack is removed
  // StopApplication ();
  Application::DoDispose();
}

uint32_t
App::GetId() const
{
  return m_appId;
}

void
App::OnInterest(shared_ptr<const Interest> interest)
{
  NS_LOG_FUNCTION(this << interest);
  m_receivedInterests(interest, this, m_face);
}

void
App::OnData(shared_ptr<const Data> data)
{
  NS_LOG_FUNCTION(this << data);
  m_receivedDatas(data, this, m_face);
}

void
App::OnNack(shared_ptr<const lp::Nack> nack)
{
  NS_LOG_FUNCTION(this << nack);

  // @TODO Implement
  m_receivedNacks(nack, this, m_face);
}

// Application Methods
void
App::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  NS_ASSERT(m_active != true);
  m_active = true;

  NS_ASSERT_MSG(GetNode()->GetObject<L3Protocol>() != 0,
                "Ndn stack should be installed on the node " << GetNode());

  // step 1. Create a face
  auto appLink = make_unique<AppLinkService>(this);
  auto transport = make_unique<NullTransport>("appFace://", "appFace://",
                                              ::ndn::nfd::FACE_SCOPE_LOCAL);
  // @TODO Consider making AppTransport instead
  m_face = std::make_shared<Face>(std::move(appLink), std::move(transport));
  m_appLink = static_cast<AppLinkService*>(m_face->getLinkService());
  m_face->setMetric(1);

  // step 2. Add face to the Ndn stack
  GetNode()->GetObject<L3Protocol>()->addFace(m_face);
}

void
App::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  if (!m_active)
    return; // don't assert here, just return

  m_active = false;

  m_face->close();
}

} // namespace ndn
} // namespace ns3
