#include "../include/GameTheoryCluster.hpp"
#include <random>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

GameTheoryCluster::GameTheoryCluster(const std::vector<std::string>& dataPointNames,
                                     const std::map<std::string, std::map<std::string, int>>& linkCostMatrix,
                                     int C)
    : dataPointNames_(dataPointNames),
      linkCostMatrix_(linkCostMatrix),
      C_(C)
{
    N_ = static_cast<int>(dataPointNames_.size());
    if (N_ > 0) {
        // Number of clusters = ceil(N / C)
        numClusters_ = static_cast<int>(std::ceil(static_cast<double>(N_) / C_));
    } else {
        numClusters_ = 0;
    }

    // Initialize assignment to -1
    clusterAssignment_.resize(N_, -1);
}

//------------------------------------------------------------------------------
// 1. Build distance matrix
//------------------------------------------------------------------------------
std::vector<std::vector<int>> GameTheoryCluster::buildDistanceMatrix() {
    // Map from nodeName to index
    std::map<std::string, int> indexMap;
    for (int i = 0; i < N_; ++i) {
        indexMap[dataPointNames_[i]] = i;
    }

    // Build NxN matrix
    std::vector<std::vector<int>> dist(N_, std::vector<int>(N_, 0));

    for (const auto& [from, costs] : linkCostMatrix_) {
        int i = indexMap[from];
        for (const auto& [to, cost] : costs) {
            int j = indexMap[to];
            dist[i][j] = cost;
            dist[j][i] = cost;  // symmetrical
        }
    }
    return dist;
}

//------------------------------------------------------------------------------
// 2. Generate initial clusters
//    We just do a random or simple distribution
//------------------------------------------------------------------------------
std::vector<std::vector<int>> GameTheoryCluster::generateInitialClusters() {
    // Basic random assignment as an example
    std::vector<int> allNodes(N_);
    std::iota(allNodes.begin(), allNodes.end(), 0);

    // Shuffle them
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(allNodes.begin(), allNodes.end(), g);

    // Make container
    std::vector<std::vector<int>> clusters(numClusters_);
    int idx = 0;
    for (int c = 0; c < numClusters_; ++c) {
        int clusterSize = (c < (N_ % numClusters_)) ? (N_ / numClusters_ + 1) : (N_ / numClusters_);
        for (int s = 0; s < clusterSize; ++s) {
            if (idx < N_) {
                clusters[c].push_back(allNodes[idx]);
                clusterAssignment_[allNodes[idx]] = c;
                ++idx;
            }
        }
    }

    return clusters;
}

//------------------------------------------------------------------------------
// 3. Repeated Best Response
//    Each node picks the cluster that yields minimal personal cost
//------------------------------------------------------------------------------
void GameTheoryCluster::repeatedBestResponse(std::vector<std::vector<int>>& clusters,
                                             const std::vector<std::vector<int>>& dist,
                                             int maxIterations)
{
    for (int iter = 0; iter < maxIterations; ++iter) {
        bool improved = false;

        // Go through every node in random order (to avoid bias)
        std::vector<int> nodeIndices(N_);
        std::iota(nodeIndices.begin(), nodeIndices.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nodeIndices.begin(), nodeIndices.end(), g);

        for (int node : nodeIndices) {
            int currentCluster = clusterAssignment_[node];
            int currentCost = nodeCostInCluster(node, clusters[currentCluster], dist);

            // Try moving node to each possible cluster
            int bestCluster = currentCluster;
            int bestCost = currentCost;

            for (int c = 0; c < numClusters_; ++c) {
                if (c == currentCluster) continue;
                // Check if c can accept more members (if needed)
                if (static_cast<int>(clusters[c].size()) >= C_) {
                    continue; // cluster is at capacity
                }

                // Potential cost if node moves to cluster c
                int newCost = 0;
                for (int member : clusters[c]) {
                    newCost += dist[node][member];
                }

                if (newCost < bestCost) {
                    bestCost = newCost;
                    bestCluster = c;
                }
            }

            // If we found a cheaper cluster, move the node
            if (bestCluster != currentCluster) {
                improved = true;

                // Remove from old cluster
                auto& oldCl = clusters[currentCluster];
                oldCl.erase(std::remove(oldCl.begin(), oldCl.end(), node), oldCl.end());

                // Add to new cluster
                clusters[bestCluster].push_back(node);
                clusterAssignment_[node] = bestCluster;
            }
        }

        // If no improvement, we stop
        if (!improved) {
            break;
        }
    }
}

//------------------------------------------------------------------------------
// 4. Helper: node's cost within a specific cluster
//------------------------------------------------------------------------------
int GameTheoryCluster::nodeCostInCluster(int nodeIndex,
                                         const std::vector<int>& cluster,
                                         const std::vector<std::vector<int>>& dist)
{
    int cost = 0;
    for (int member : cluster) {
        if (member != nodeIndex) {
            cost += dist[nodeIndex][member];
        }
    }
    return cost;
}

//------------------------------------------------------------------------------
// 5. Convert final cluster indices to names
//------------------------------------------------------------------------------
std::vector<std::vector<std::string>>
GameTheoryCluster::finalizeClusters(const std::vector<std::vector<int>>& clusters)
{
    std::vector<std::vector<std::string>> namedClusters(numClusters_);
    for (int c = 0; c < numClusters_; ++c) {
        for (int idx : clusters[c]) {
            namedClusters[c].push_back(dataPointNames_[idx]);
        }
    }
    return namedClusters;
}

//------------------------------------------------------------------------------
// 6. Compute global cost for reference
//------------------------------------------------------------------------------
int GameTheoryCluster::computeGlobalCost(const std::vector<std::vector<int>>& clusters,
                                         const std::vector<std::vector<int>>& dist)
{
    int totalCost = 0;
    for (const auto& cl : clusters) {
        for (size_t i = 0; i < cl.size(); ++i) {
            for (size_t j = i + 1; j < cl.size(); ++j) {
                totalCost += dist[cl[i]][cl[j]];
            }
        }
    }
    return totalCost;
}

//------------------------------------------------------------------------------
// 7. Main run function
//------------------------------------------------------------------------------
std::vector<std::vector<std::string>> GameTheoryCluster::runGameTheoryClustering(int maxIterations /*=100*/)
{
    // Edge case
    if (N_ == 0 || numClusters_ == 0) {
        return {};
    }

    // Build the distance matrix once
    std::vector<std::vector<int>> dist = buildDistanceMatrix();

    // Generate initial clusters
    std::vector<std::vector<int>> clusters = generateInitialClusters();

    // Repeated best response
    repeatedBestResponse(clusters, dist, maxIterations);

    // Compute final cost for reference
    int finalCost = computeGlobalCost(clusters, dist);
    std::cout << "Final global cost after game-theoretic approach: " << finalCost << std::endl;

    // Convert to names
    auto namedClusters = finalizeClusters(clusters);

    // Optionally print them
    for (int c = 0; c < numClusters_; ++c) {
        std::cout << "Cluster " << c << " [size = " << clusters[c].size() << "]: ";
        for (auto& nm : namedClusters[c]) {
            std::cout << nm << " ";
        }
        std::cout << std::endl;
    }

    return namedClusters;
}