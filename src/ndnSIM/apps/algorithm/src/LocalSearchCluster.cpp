#include "../include/LocalSearchCluster.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <ctime>
#include <random>
#include <climits>
#include <cstdlib> // for rand, RAND_MAX
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
// Constructor: Initialize the object with input node names, the link cost matrix,
// and the parameter C (maximum size of a cluster). This constructor also computes
// the total number of nodes (N_) and the number of clusters based on N_ and C.
 //------------------------------------------------------------------------------
LocalSearchCluster::LocalSearchCluster(const std::vector<std::string>& dataPointNames,
                                       const std::map<std::string, std::map<std::string, int>>& linkCostMatrix,
                                       int C)
    : dataPointNames_(dataPointNames), linkCostMatrix_(linkCostMatrix), C_(C)
{
    // Total number of data points
    N_ = static_cast<int>(dataPointNames_.size());
    // Compute the number of clusters required (each cluster has up to C_ nodes)
    numClusters_ = (N_ > 0) ? static_cast<int>(std::ceil(N_ / static_cast<double>(C_))) : 0;
    // Initialize cluster assignments (-1 means not assigned)
    clusterAssignment_.resize(N_, -1);
    // Prepare final clusters container (each with a list of node names)
    clusters_.resize(numClusters_);
}

//------------------------------------------------------------------------------
// runClustering: Main entry point for clustering.
// This function runs several random restarts and for each try, it builds the distance
// matrix, generates the initial clusters, and then refines them with local search
// (using simulated annealing in the optimization loop below to allow occasional uphill moves).
// Finally, the best (lowest cost) clustering is selected.
//------------------------------------------------------------------------------
std::vector<std::vector<std::string>> LocalSearchCluster::runClustering(int numRestarts) {
    if (N_ == 0)
        return {};

    // Build the distance matrix (symmetric matrix of node-to-node distances)
    auto dist = buildDistanceMatrix();
    int bestGlobalCost = INT_MAX;
    std::vector<std::vector<int>> bestClusterIndices;

    // Use max between provided restarts and 20 for robust results.
    const int effectiveRestarts = std::max(numRestarts, 1);

    for (int attempt = 0; attempt < effectiveRestarts; ++attempt) {
        // 1. Generate initial clusters using a greedy method.
        auto clusterIndices = generateEnhancedInitialClusters(dist);
        
        // 2. Compute the initial sum of pairwise distances for each cluster.
        auto clusterSums = computeInitialClusterSums(clusterIndices, dist);

        // Compute initial global cost.
        int initialGlobalCost = computeGlobalCost(clusterIndices, dist);

        // --- Print the initial clustering result and start cost ---
        std::cout << "Initial clustering attempt " << attempt << ":\n";
        for (int c = 0; c < numClusters_; ++c) {
            std::cout << "Cluster " << c << " contains: ";
            for (int node : clusterIndices[c]) {
                std::cout << dataPointNames_[node] << " ";
            }
            std::cout << "\nLocal cost for cluster " << c << ": " << clusterSums[c] << "\n";
        }
        std::cout << "Initial Global cost: " << initialGlobalCost << "\n\n";
        // --- End of printing ---

        // 3. Restructure clusters using random moves to improve cost.
        performClusterRestructuring(clusterIndices, clusterSums, dist);
        // 4. Perform consecutive pair optimization.
        optimizeConsecutivePairs(clusterIndices, clusterSums, dist);
        // 5. Optimize via simulated annealing based local moves.
        runOptimizationLoop(clusterIndices, clusterSums, dist);

        // Compute the total cost for the current clustering solution.
        int currentCost = computeGlobalCost(clusterIndices, dist);
        if (currentCost < bestGlobalCost) {
            bestGlobalCost = currentCost;
            bestClusterIndices = clusterIndices;
        }
    }

    // Convert final cluster indices (which are integer node indices) to node names.
    finalizeClusters(bestClusterIndices);
    return clusters_;
}

//------------------------------------------------------------------------------
// getClusters: Accessor for final clusters (list of node names grouped by cluster).
//------------------------------------------------------------------------------
std::vector<std::vector<std::string>> LocalSearchCluster::getClusters() const {
    return clusters_;
}

//------------------------------------------------------------------------------
// buildDistanceMatrix: Create and return a symmetric matrix of pairwise distances.
// The distance from a node to itself is zero.
//------------------------------------------------------------------------------
std::vector<std::vector<int>> LocalSearchCluster::buildDistanceMatrix() {
    // Map each node name to an index.
    std::map<std::string, int> nodeIndex;
    for (int i = 0; i < N_; ++i) {
        nodeIndex[dataPointNames_[i]] = i;
    }

    // Create N_ x N_ matrix initialized to 0.
    std::vector<std::vector<int>> dist(N_, std::vector<int>(N_, 0));

    // Fill the matrix using the link cost matrix (assumed symmetric).
    for (const auto& [from, costs] : linkCostMatrix_) {
        int i = nodeIndex[from];
        for (const auto& [to, cost] : costs) {
            int j = nodeIndex[to];
            dist[i][j] = cost;
            dist[j][i] = cost;
        }
    }

    return dist;
}

//------------------------------------------------------------------------------
// generateEnhancedInitialClusters: Create initial clusters by first shuffling 
// the node indices and then greedily assigning them to clusters based on the 
// minimum additional cost to that cluster.
//------------------------------------------------------------------------------
std::vector<std::vector<int>> LocalSearchCluster::generateEnhancedInitialClusters(
    const std::vector<std::vector<int>>& dist) 
{
    // Create a sorted list of node indices from 0 to N_-1.
    std::vector<int> nodes(N_);
    std::iota(nodes.begin(), nodes.end(), 0);

    // Shuffle the indices for randomness.
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(nodes.begin(), nodes.end(), g);

    // Initialize clusters by placing the first numClusters_ nodes as seeds.
    std::vector<std::vector<int>> clusterIndices(numClusters_);
    for (int i = 0; i < numClusters_; ++i) {
        clusterIndices[i].push_back(nodes[i]);
        clusterAssignment_[nodes[i]] = i;
    }

    // Compute group sizes: m nodes in each cluster, with r clusters having one extra.
    int m = N_ / numClusters_;
    int r = N_ % numClusters_;

    // Greedy assignment for remaining nodes.
    for (size_t i = numClusters_; i < nodes.size(); ++i) {
        int node = nodes[i];
        int bestCluster = -1;
        int minCost = INT_MAX;

        // Try adding the node to each cluster, if the maximum cluster size is not reached.
        for (int c = 0; c < numClusters_; ++c) {
            const int max_size = (c < r) ? m + 1 : m;
            if (clusterIndices[c].size() >= max_size)
                continue;

            // Calculate the cost for adding this node to the cluster.
            int cost = 0;
            for (int member : clusterIndices[c]) {
                cost += dist[node][member];
            }

            if (cost < minCost) {
                minCost = cost;
                bestCluster = c;
            }
        }

        if (bestCluster != -1) {
            clusterIndices[bestCluster].push_back(node);
            clusterAssignment_[node] = bestCluster;
        }
    }

    return clusterIndices;
}

//------------------------------------------------------------------------------
// computeInitialClusterSums: For each cluster, compute the sum of all pairwise 
// distances between the nodes. This sum represents the "cost" of the cluster.
//------------------------------------------------------------------------------
std::vector<int> LocalSearchCluster::computeInitialClusterSums(
    const std::vector<std::vector<int>>& clusterIndices,
    const std::vector<std::vector<int>>& dist)
{
    std::vector<int> clusterSums(numClusters_, 0);

    for (int c = 0; c < numClusters_; ++c) {
        const auto& cl = clusterIndices[c];
        int sum = 0;
        for (int i = 0; i < static_cast<int>(cl.size()); ++i) {
            for (int j = i + 1; j < static_cast<int>(cl.size()); ++j) {
                sum += dist[cl[i]][cl[j]];
            }
        }
        clusterSums[c] = sum;
    }

    return clusterSums;
}

//------------------------------------------------------------------------------
// runOptimizationLoop: Local search routine that iteratively performs single-node
// moves and additional pair-swap moves using simulated annealing.
//------------------------------------------------------------------------------
void LocalSearchCluster::runOptimizationLoop(
    std::vector<std::vector<int>>& clusterIndices,
    std::vector<int>& clusterSums,
    const std::vector<std::vector<int>>& dist)
{
    // Improved simulated annealing parameters (tune as needed)
    double T = 150.0;                   // Increased initial temperature
    double coolingRate = 0.995;           // Slightly slower cooling rate
    int iterationsAtTemp = 250;           // More moves attempted per temperature level

    std::vector<int> allClusters(numClusters_);
    std::iota(allClusters.begin(), allClusters.end(), 0);

    int currentGlobalCost = computeGlobalCost(clusterIndices, dist);

    // Continue until temperature becomes very low.
    while (T > 1e-3) {
        // Single-node move attempts.
        for (int iter = 0; iter < iterationsAtTemp; ++iter) {
            int source = allClusters[rand() % numClusters_];
            if (clusterIndices[source].empty())
                continue;
            int node = clusterIndices[source][rand() % clusterIndices[source].size()];
            int target = allClusters[rand() % numClusters_];
            if (source == target || clusterIndices[target].size() >= static_cast<size_t>(C_))
                continue;

            int currentCost = calculateNodeCost(node, clusterIndices[source], dist);
            int newCost = calculateNodeCost(node, clusterIndices[target], dist);
            int delta = newCost - currentCost;

            // Accept move if it improves cost or with probability exp(-delta/T)
            if (delta < 0 || (std::exp(-delta / T) > (static_cast<double>(rand()) / RAND_MAX))) {
                // Execute the move.
                auto &src = clusterIndices[source];
                src.erase(std::remove(src.begin(), src.end(), node), src.end());
                clusterIndices[target].push_back(node);
                clusterAssignment_[node] = target;
                currentGlobalCost = computeGlobalCost(clusterIndices, dist);
            }
        }
        
        // Additional pair-swap moves: try exchanging one node from each of two clusters.
        for (int iter = 0; iter < iterationsAtTemp / 2; ++iter) {
            // Randomly pick two different clusters.
            int cl1 = allClusters[rand() % numClusters_];
            int cl2 = allClusters[rand() % numClusters_];
            if (cl1 == cl2 || clusterIndices[cl1].empty() || clusterIndices[cl2].empty())
                continue;

            // Pick one random node from each cluster.
            int node1 = clusterIndices[cl1][rand() % clusterIndices[cl1].size()];
            int node2 = clusterIndices[cl2][rand() % clusterIndices[cl2].size()];
            
            // Calculate the current cost for node1 in cl1 and node2 in cl2.
            int currentCostNode1 = calculateNodeCost(node1, clusterIndices[cl1], dist);
            int currentCostNode2 = calculateNodeCost(node2, clusterIndices[cl2], dist);
            
            // Now compute the new cost if node1 is placed in cl2 and node2 in cl1.
            int newCostNode1 = calculateNodeCost(node1, clusterIndices[cl2], dist);
            int newCostNode2 = calculateNodeCost(node2, clusterIndices[cl1], dist);
            
            int deltaSwap = (newCostNode1 + newCostNode2) - (currentCostNode1 + currentCostNode2);
            
            // Accept the swap if it improves the solution or via simulated annealing chance.
            if (deltaSwap < 0 || (std::exp(-deltaSwap / T) > (static_cast<double>(rand()) / RAND_MAX))) {
                // Perform the swap.
                auto &vec1 = clusterIndices[cl1];
                auto &vec2 = clusterIndices[cl2];
                vec1.erase(std::remove(vec1.begin(), vec1.end(), node1), vec1.end());
                vec2.erase(std::remove(vec2.begin(), vec2.end(), node2), vec2.end());
                vec1.push_back(node2);
                vec2.push_back(node1);
                clusterAssignment_[node1] = cl2;
                clusterAssignment_[node2] = cl1;
                currentGlobalCost = computeGlobalCost(clusterIndices, dist);
            }
        }
        // Cool down the temperature.
        T *= coolingRate;
    }
}

//------------------------------------------------------------------------------
// finalizeClusters: Convert the clusters that are stored as indices into clusters
// containing the actual data point names.
//------------------------------------------------------------------------------
void LocalSearchCluster::finalizeClusters(const std::vector<std::vector<int>>& clusterIndices) {
    // Clear any previous assignments
    for (auto& cluster : clusters_) {
        cluster.clear();
    }

    // Populate clusters with data point names
    for (int c = 0; c < numClusters_; ++c) {
        for (int node : clusterIndices[c]) {
            clusters_[c].push_back(dataPointNames_[node]);
        }
    }
}

//------------------------------------------------------------------------------
// computeGlobalCost: Calculate the total cost of all clusters by summing the cost
// of each cluster's pairwise distances.
//------------------------------------------------------------------------------
int LocalSearchCluster::computeGlobalCost(
    const std::vector<std::vector<int>>& clusterIndices,
    const std::vector<std::vector<int>>& dist) const
{
    int totalCost = 0;
    for (const auto& cl : clusterIndices) {
        for (size_t i = 0; i < cl.size(); ++i) {
            for (size_t j = i + 1; j < cl.size(); ++j) {
                totalCost += dist[cl[i]][cl[j]];
            }
        }
    }
    return totalCost;
}

//------------------------------------------------------------------------------
// calculateNodeCost: Compute the cost for a single node with respect to a given cluster.
// If an excludeNode is provided, that node is ignored in the summation (default is -1).
//------------------------------------------------------------------------------
int LocalSearchCluster::calculateNodeCost(int node, const std::vector<int>& cluster, 
                                        const std::vector<std::vector<int>>& dist,
                                        int excludeNode) const 
{
    int cost = 0;
    for (int other : cluster) {
        if (other != node && other != excludeNode) {
            cost += dist[node][other];
        }
    }
    return cost;
}

//------------------------------------------------------------------------------
// moveNode: Move a node from the source cluster to the target cluster.
// This updates the clusters, the cumulative cluster cost sums, and the node's assignment.
//------------------------------------------------------------------------------
void LocalSearchCluster::moveNode(int node, int source, int target,
                                std::vector<std::vector<int>>& clusters,
                                std::vector<int>& clusterSums,
                                const std::vector<std::vector<int>>& dist) 
{
    // Remove the node from the source cluster.
    auto& src = clusters[source];
    src.erase(std::remove(src.begin(), src.end(), node), src.end());
    clusterSums[source] -= calculateNodeCost(node, src, dist);

    // Add the node to the target cluster.
    auto& tgt = clusters[target];
    tgt.push_back(node);
    clusterSums[target] += calculateNodeCost(node, tgt, dist);
    clusterAssignment_[node] = target;
}

//------------------------------------------------------------------------------
// performClusterRestructuring: Perform random restructuring moves between clusters.
// Here, two random clusters are chosen and random nodes are exchanged if the
// total cost (delta) is reduced.
//------------------------------------------------------------------------------
void LocalSearchCluster::performClusterRestructuring(
    std::vector<std::vector<int>>& clusters,
    std::vector<int>& clusterSums,
    const std::vector<std::vector<int>>& dist)
{
    const int iterations = 50;
    std::mt19937 gen(std::random_device{}());
    
    for (int i = 0; i < iterations; ++i) {
        // Randomly select two different clusters.
        int c1 = rand() % numClusters_;
        int c2 = rand() % numClusters_;
        if (c1 == c2)
            continue;

        // If both clusters have at least one node, attempt an exchange.
        if (!clusters[c1].empty() && !clusters[c2].empty()) {
            int node1 = clusters[c1][rand() % clusters[c1].size()];
            int node2 = clusters[c2][rand() % clusters[c2].size()];
            
            // Calculate cost delta for moving node1 and node2.
            int delta = calculateMoveDelta(node1, c1, c2, clusters, dist) +
                        calculateMoveDelta(node2, c2, c1, clusters, dist);

            // If the total cost reduction is positive, execute the exchange.
            if (delta < 0) {
                moveNode(node1, c1, c2, clusters, clusterSums, dist);
                moveNode(node2, c2, c1, clusters, clusterSums, dist);
            }
        }
    }
}

//------------------------------------------------------------------------------
// optimizeConsecutivePairs: Special optimization targeting consecutive nodes.
// If two consecutive nodes are in different clusters, the algorithm will evaluate
// if moving both into a common cluster reduces the overall cost, and performs that move.
//------------------------------------------------------------------------------
void LocalSearchCluster::optimizeConsecutivePairs(
    std::vector<std::vector<int>>& clusters,
    std::vector<int>& clusterSums,
    const std::vector<std::vector<int>>& dist)
{
    for (int node = 0; node < N_ - 1; ++node) {
        int clusterA = clusterAssignment_[node];
        int clusterB = clusterAssignment_[node + 1];
        if (clusterA == clusterB)
            continue;

        // Calculate the current cost of the two nodes in their own clusters.
        int currentCost = calculateNodeCost(node, clusters[clusterA], dist) +
                          calculateNodeCost(node + 1, clusters[clusterB], dist);
        
        // Calculate the potential cost if they are both moved to the opposite cluster.
        int potentialCost = calculateNodeCost(node, clusters[clusterB], dist, node + 1) +
                             calculateNodeCost(node + 1, clusters[clusterA], dist, node);
        
        // If cost reduction is achieved, move both nodes.
        if (potentialCost < currentCost) {
            int targetCluster = (clusters[clusterA].size() < clusters[clusterB].size()) ? clusterA : clusterB;
            moveNode(node, clusterAssignment_[node], targetCluster, clusters, clusterSums, dist);
            moveNode(node + 1, clusterAssignment_[node + 1], targetCluster, clusters, clusterSums, dist);
        }
    }
}

//------------------------------------------------------------------------------
// calculateMoveDelta: Calculate the difference in cost when moving a node from
// its source cluster to a target cluster. A negative delta implies an improvement.
//------------------------------------------------------------------------------
int LocalSearchCluster::calculateMoveDelta(int node, int source, int target,
                                         const std::vector<std::vector<int>>& clusters,
                                         const std::vector<std::vector<int>>& dist) const
{
    int oldCost = calculateNodeCost(node, clusters[source], dist);
    int newCost = calculateNodeCost(node, clusters[target], dist);
    return newCost - oldCost;
}