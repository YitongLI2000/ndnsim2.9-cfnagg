#ifndef GAME_THEORY_CLUSTER_H
#define GAME_THEORY_CLUSTER_H

#include <vector>
#include <map>
#include <string>

class GameTheoryCluster {
public:
    GameTheoryCluster(const std::vector<std::string>& dataPointNames,
                      const std::map<std::string, std::map<std::string, int>>& linkCostMatrix,
                      int C);

    // Main run function: repeated best response
    std::vector<std::vector<std::string>> runGameTheoryClustering(int maxIterations = 100);

private:
    // Build the distance matrix
    std::vector<std::vector<int>> buildDistanceMatrix();

    // Initialize the clustering (like your previous code)
    std::vector<std::vector<int>> generateInitialClusters();

    // Repeated best-response update
    void repeatedBestResponse(std::vector<std::vector<int>>& clusters,
                              const std::vector<std::vector<int>>& dist,
                              int maxIterations);

    // Helper to compute cost of a node in a specific cluster
    int nodeCostInCluster(int nodeIndex,
                          const std::vector<int>& cluster,
                          const std::vector<std::vector<int>>& dist);

    // Convert final cluster indices to names
    std::vector<std::vector<std::string>> finalizeClusters(const std::vector<std::vector<int>>& clusters);

    // Compute the overall cost for reference
    int computeGlobalCost(const std::vector<std::vector<int>>& clusters,
                          const std::vector<std::vector<int>>& dist);

private:
    std::vector<std::string> dataPointNames_;
    std::map<std::string, std::map<std::string, int>> linkCostMatrix_;
    int C_ = 0;     // cluster size
    int N_ = 0;     // number of data points
    int numClusters_ = 0;

    // Optional: store each node’s cluster assignment for convenience
    std::vector<int> clusterAssignment_;
};

#endif // GAME_THEORY_CLUSTER_H