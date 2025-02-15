#include "../include/AggregationTree.hpp"
#include "../include/regularized_k_means.hpp"
#include "../include/LocalSearchCluster.hpp"
#include "../include/GameTheoryCluster.hpp"
#include "../utility/utility.hpp"

#include <iostream>
#include <cmath> // For ceil
#include <numeric> // For std::accumulate
#include <cassert> // For assert
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <limits>
#include <unordered_map>
#include <climits>
#include <stdexcept>
#include <algorithm>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <ctime>

#include <stdexcept>


// Zhuoxu: In order to use the constructor, we need to input a file name to initialize the AggregationTree object.
AggregationTree::AggregationTree(std::string file){
    filename = file;

    // Zhuoxu: FullList is used to get the nodes of cluster head candidates. After CH is chosen, it will be removed from CHList. This is how fullList is used. 
    fullList = Utility::getContextInfo(filename);
    CHList = fullList;

    // Zhuoxu: The linkCostMatrix is what we need to mark as the input of the game theory algorithm.
    linkCostMatrix = Utility::GetAllLinkCost(filename);
}


std::string AggregationTree::findCH(std::vector<std::string> clusterNodes, std::vector<std::string> clusterHeadCandidate, std::string client) {

    std::string CH = client;
    // Initiate a large enough cost
    int leastCost = 1000;

    for (const auto& headCandidate : clusterHeadCandidate) {
        bool canBeCH = true;

        for (const auto& node : clusterNodes) {
            if (linkCostMatrix[node][client] < linkCostMatrix[node][headCandidate]) {
                canBeCH = false;
                break;
            }
        }

        // This candidate is closer to client
        long long totalCost = 0;
        if (canBeCH) {
            for (const auto& node : clusterNodes) {
                totalCost += linkCostMatrix[node][headCandidate];
            }
            int averageCost = static_cast<int>(totalCost / clusterNodes.size());

            if (averageCost < leastCost) {
                leastCost = averageCost;
                CH = headCandidate;
            }
        }
    }

    if (CH == client) {
        std::cerr << "No CH is found for current cluster!!!!!!!!!!!" << std::endl;
        return CH;
    } else {
        std::cout << "CH " << CH << " is chosen." << std::endl;
        return CH;
    }
}

std::vector<std::vector<std::string>> 
AggregationTree::runBKM(std::vector<std::string>& dataPointNames, int numClusters){
    int N = dataPointNames.size();

    // Create a vector to store cluster assignments
    std::vector<int> clusterAssignment(N);
    for (int i = 0; i < N; ++i) {
        clusterAssignment[i] = i % numClusters; // Cluster assignment based on modulus operation
    }

    // Create a map of clusters to their data points, store data point's ID inside cluster's vector
    std::vector<std::vector<std::string>> clusters(numClusters);
    for (int i = 0; i < N; ++i) {
        clusters[clusterAssignment[i]].push_back(dataPointNames[i]);
    }

    // BKM begins...
    int runs = 1; // number of repeat
    int threads = -1; // default, auto-detect
    bool no_warm_start = false; // must have warm start
    unsigned int seed = std::random_device{}(); // seed for initialization
    RegularizedKMeans::InitMethod init_method = RegularizedKMeans::InitMethod::kForgy;

    double result;
    KMeans* k_means;
    auto* rkm = new RegularizedKMeans(
            dataPointNames, numClusters,linkCostMatrix, init_method, !no_warm_start,
            threads, seed);
    result = rkm->SolveHard();
    k_means = rkm;
    //delete k_means;

    // BKM finish...

    return k_means->clusters;
}

bool AggregationTree::aggregationTreeConstruction(std::vector<std::string> dataPointNames, int C) {

    // Compute N
    int N = dataPointNames.size();

    // Compute the number of clusters k
    int numClusters = static_cast<int>(ceil(static_cast<double>(N) / C));

    std::vector<std::vector<std::string>> newCluster;

    bool isLocalSearchCluster = true;
    if(isLocalSearchCluster)
        newCluster = LocalSearchCluster(dataPointNames, linkCostMatrix, C).runClustering();
    else
        // newCluster = this->runBKM(dataPointNames, numClusters);
        newCluster = GameTheoryCluster(dataPointNames, linkCostMatrix, C).runGameTheoryClustering(100);

    PrintClusterCosts(newCluster, linkCostMatrix);

    // Zhuoxu: Todo, the output of the Game Theory algorithm should be std::vector<std::vector<std::string>> newCluster;

    // Construct the nodeList for CH allocation
    int i = 0;
    std::cout << "\nIterating new clusters." << std::endl;
    for (const auto& iteCluster: newCluster) {
        std::cout << "Cluster " << i << " contains the following nodes:" <<std::endl;
        for (const auto& iteNode: iteCluster) {
            CHList.erase(std::remove(CHList.begin(), CHList.end(), iteNode), CHList.end());
            std::cout << iteNode << " ";
        }
        std::cout << std::endl;
        ++i;
    }


    std::cout << "\nCurrent CH candidates: " << std::endl;
    for (const auto& item : CHList) {
        std::cout << item << std::endl;
    }

    // Start CH allocation
    std::vector<std::string> newDataPoints;
    std::cout << "\nStarting CH allocation." << std::endl;
    for (const auto& clusterNodes : newCluster) {
        std::string clusterHead = findCH(clusterNodes, CHList, globalClient);
         // "globalClient" doesn't exist in candidate list, if CH == globalClient, it means no CH is found
        if (clusterHead != globalClient) {
            CHList.erase(std::remove(CHList.begin(), CHList.end(), clusterHead), CHList.end());
            aggregationAllocation[clusterHead] = clusterNodes;
            newDataPoints.push_back(clusterHead); // If newDataPoints are more than C, perform tree construction for next layer
        }
        else {
            std::cout << "No cluster head found for current cluster, combine them into sub-tree." << std::endl;
            noCHTree.push_back(clusterNodes);
        }

    }

    std::cout << "\nThe rest CH candidates after CH allocation: " << std::endl;
    for (const auto& item : CHList) {
        std::cout << item << std::endl;
    }

    if (newDataPoints.size() < C){

        if (newDataPoints.empty()) // If all clusters can't find CH, allocate the first sub-tree to aggregationAllocation for first round, then iterate other sub-trees in later rounds
        {
            // Move the first sub-tree to "aggregationAllocation"
            const auto& firstSubTree = noCHTree[0];
            aggregationAllocation[globalClient] = firstSubTree;
            noCHTree.erase(noCHTree.begin());
            return true;
        } else // If some clusters can find CH, put them into aggregationAllocation for first round, iterate other sub-trees in later rounds
        {
            aggregationAllocation[globalClient] = newDataPoints;
            return true;
        }
    } else {
        aggregationTreeConstruction(newDataPoints, C);
    }
}

// This function calculates and prints both local cost (sum of pairwise distances within each cluster) 
// and global cost (sum of all local costs).
// If a distance entry is missing in linkCostMatrix, it is treated as zero by default.
void 
AggregationTree::PrintClusterCosts(
    const std::vector<std::vector<std::string>>& newCluster,
    const std::map<std::string, std::map<std::string, int>>& linkCostMatrix
) {
    int globalCost = 0;

    // Iterate through each cluster
    for (size_t c = 0; c < newCluster.size(); ++c) {
        int localCost = 0;
        const auto& cluster = newCluster[c];

        // Compute pairwise distances for all elements in the cluster
        for (size_t i = 0; i < cluster.size(); ++i) {
            for (size_t j = i + 1; j < cluster.size(); ++j) {
                const std::string& nodeA = cluster[i];
                const std::string& nodeB = cluster[j];

                // If distance is present, add it
                if (linkCostMatrix.find(nodeA) != linkCostMatrix.end() 
                    && linkCostMatrix.at(nodeA).find(nodeB) != linkCostMatrix.at(nodeA).end()) 
                {
                    localCost += linkCostMatrix.at(nodeA).at(nodeB);
                } 
                else {
                    // Optional: handle missing distance (currently assumed 0)
                    // Example:
                    // std::cerr << "Warning: Missing distance between " 
                    //           << nodeA << " and " << nodeB << std::endl;
                    // Throw a runtime error if missing
                    throw std::runtime_error("Distance between " + nodeA +
                                             " and " + nodeB +
                                             " is missing in linkCostMatrix.");
                }
            }
        }

        std::cout << "Local cost for cluster " << c << ": " << localCost << std::endl;
        globalCost += localCost;
    }

    std::cout << "Global cost (sum of local costs): " << globalCost << std::endl;
}

/// This function takes in a link cost matrix (map of maps) and a filename,
/// then writes each (source, target, cost) triplet to the specified file in CSV format.
/// Only the pairs where the target node name starts with "pro" are processed,
/// and for each source, the output is sorted so that the smallest distance appears first.
void 
AggregationTree::writeLinkCostsToFile(const std::string& filename)
{
    // Open the output file.
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Write header row (optional).
    outFile << "Source,Target,Cost\n";

    // Iterate over each source node in the link cost matrix.
    for (const auto& sourcePair : linkCostMatrix) {
        const std::string& source = sourcePair.first;

        // Create a vector to hold only the target nodes that start with "pro".
        std::vector<std::pair<std::string, int>> proTargets;
        for (const auto& targetPair : sourcePair.second) {
            const std::string& target = targetPair.first;
            // Filter: include only targets that start with "pro"
            if (target.compare(0, 3, "pro") == 0) {
                proTargets.emplace_back(target, targetPair.second);
            }
        }

        // Sort the matching target pairs based on the cost in ascending order.
        std::sort(proTargets.begin(), proTargets.end(),
                  [](const std::pair<std::string,int>& a, const std::pair<std::string,int>& b)
                  {
                      return a.second < b.second;
                  });

        // Write the sorted results for this source to the file.
        for (const auto& p : proTargets) {
            outFile << source << "," << p.first << "," << p.second << "\n";
        }
    }

    outFile.close();
    std::cout << "Link costs have been written to " << filename << std::endl;
}