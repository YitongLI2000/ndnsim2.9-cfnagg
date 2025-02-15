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

#ifndef NDN_APP_H
#define NDN_APP_H

#include <filesystem> // Added by Yitong
#include <chrono> // Added by Yitong
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <optional>

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/ndn-app-link-service.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/face.hpp"

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"


namespace ns3 {

class Packet;

namespace ndn {


enum CcAlgorithm {
  AIMD,
  CUBIC
};  

/**
 * \ingroup ndn
 * \defgroup ndn-apps NDN applications
 */
/**
 * @ingroup ndn-apps
 * @brief Base class that all NDN applications should be derived from.
 *
 * The class implements virtual calls onInterest, onNack, and onData
 */
class App : public Application {
public:
  static TypeId
  GetTypeId();

  /**
   * @brief Default constructor
   */
  App();
  virtual ~App();

  /**
   * @brief Get application ID (ID of applications face)
   */
  uint32_t
  GetId() const;

  /**
   * @brief Method that will be called every time new Interest arrives
   */
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  /**
   * @brief Method that will be called every time new Data arrives
   */
  virtual void
  OnData(shared_ptr<const Data> data);

   /**
   * @brief Method that will be called every time new Nack arrives
   */
  virtual void
  OnNack(shared_ptr<const lp::Nack> nack);


  // New design to implement algorithm
  void
  ConstructAggregationTree();




public:
  typedef void (*InterestTraceCallback)(shared_ptr<const Interest>, Ptr<App>, shared_ptr<Face>);
  typedef void (*DataTraceCallback)(shared_ptr<const Data>, Ptr<App>, shared_ptr<Face>);
  typedef void (*NackTraceCallback)(shared_ptr<const lp::Nack>, Ptr<App>, shared_ptr<Face>);

protected:
  virtual void
  DoInitialize();

  virtual void
  DoDispose();

  // inherited from Application base class. Originally they were private
  virtual void
  StartApplication(); ///< @brief Called at time specified by Start

  virtual void
  StopApplication(); ///< @brief Called at time specified by Stop

  std::set<std::string>
  findLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap);

  std::map<std::string, std::set<std::string>>
  getLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap);

  int findRoundIndex(const std::vector<std::vector<std::string>>& roundVec, const std::string& target);

  void CheckDirectoryExist(const std::string& path);

  void OpenFile(const std::string& filename);

// New design for tree topology to get child node info
public:
    std::map<std::string, std::vector<std::string>> m_linkInfo;

    // Throughput/aggregation tree log file
    std::string throughput_recorder = "src/ndnSIM/results/logs/throughput.txt"; // "totalInterestThroughput", "totalDataThroughput", "total time"
    std::string aggTree_recorder = "src/ndnSIM/results/logs/aggTree.txt";
    std::string result_recorder = "src/ndnSIM/results/logs/result.txt";

protected:

  bool m_active; ///< @brief Flag to indicate that application is active (set by StartApplication and StopApplication)
  shared_ptr<Face> m_face;
  AppLinkService* m_appLink;

  CcAlgorithm m_ccAlgorithm;

  uint32_t m_appId;

  TracedCallback<shared_ptr<const Interest>, Ptr<App>, shared_ptr<Face>>
    m_receivedInterests; ///< @brief App-level trace of received Interests

  TracedCallback<shared_ptr<const Data>, Ptr<App>, shared_ptr<Face>>
    m_receivedDatas; ///< @brief App-level trace of received Data

  TracedCallback<shared_ptr<const lp::Nack>, Ptr<App>, shared_ptr<Face>>
    m_receivedNacks; ///< @brief App-level trace of received Nacks

  TracedCallback<shared_ptr<const Interest>, Ptr<App>, shared_ptr<Face>>
    m_transmittedInterests; ///< @brief App-level trace of transmitted Interests

  TracedCallback<shared_ptr<const Data>, Ptr<App>, shared_ptr<Face>>
    m_transmittedDatas; ///< @brief App-level trace of transmitted Data

  TracedCallback<shared_ptr<const lp::Nack>, Ptr<App>, shared_ptr<Face>>
    m_transmittedNacks; ///< @brief App-level trace of transmitted Nacks
};

} // namespace ndn
} // namespace ns3

#endif // NDN_APP_H
