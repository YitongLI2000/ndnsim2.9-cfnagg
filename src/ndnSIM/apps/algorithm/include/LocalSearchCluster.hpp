#ifndef LOCAL_SEARCH_CLUSTER_H
#define LOCAL_SEARCH_CLUSTER_H

#include <vector>
#include <map>
#include <string>

class LocalSearchCluster {
public:
    // Constructor: Accepts input data (node names and link cost matrix) and parameter C.
    // C represents the maximum allowed size of each cluster.
    LocalSearchCluster(const std::vector<std::string>& dataPointNames,
                       const std::map<std::string, std::map<std::string, int>>& linkCostMatrix,
                       int C);

    // Main entry point for clustering.
    // Runs the clustering process (with multiple random restarts) and returns the clusters (node names grouped by cluster).
    // The local search now uses simulated annealing with additional pair-swap moves.
    std::vector<std::vector<std::string>> runClustering(int numRestarts = 1);

    // Accessor to retrieve the final clusters.
    std::vector<std::vector<std::string>> getClusters() const;

private:
    // 1. Build the distance matrix (symmetric matrix based on link costs).
    std::vector<std::vector<int>> buildDistanceMatrix();

    // 2. Generate initial clusters using a greedy approach.
    std::vector<std::vector<int>> generateEnhancedInitialClusters(const std::vector<std::vector<int>>& dist);

    // 3. Compute the initial cost summary for each cluster (sum of pairwise distances).
    std::vector<int> computeInitialClusterSums(const std::vector<std::vector<int>>& clusterIndices,
                                               const std::vector<std::vector<int>>& dist);

    // 4. Local optimization loop that employs simulated annealing and pair-swap moves.
    void runOptimizationLoop(std::vector<std::vector<int>>& clusterIndices,
                             std::vector<int>& clusterSums,
                             const std::vector<std::vector<int>>& dist);

    // 5. Convert cluster indices (node indices) into final clusters of node names.
    void finalizeClusters(const std::vector<std::vector<int>>& clusterIndices);

    // 6. Compute the total global cost (sum of intra-cluster distances).
    int computeGlobalCost(const std::vector<std::vector<int>>& clusterIndices,
                          const std::vector<std::vector<int>>& dist) const;

    // ---------------- Helper Methods ----------------

    // Compute the cost for a single node with respect to a given cluster.
    // If an excludeNode is provided, that node is ignored in the summation (default is -1).
    int calculateNodeCost(int node, const std::vector<int>& cluster, 
                          const std::vector<std::vector<int>>& dist,
                          int excludeNode = -1) const;

    // Move a node from the source cluster to the target cluster.
    void moveNode(int node, int source, int target,
                  std::vector<std::vector<int>>& clusters,
                  std::vector<int>& clusterSums,
                  const std::vector<std::vector<int>>& dist);

    // Restructure clusters by performing random exchanges between clusters.
    void performClusterRestructuring(std::vector<std::vector<int>>& clusters,
                                     std::vector<int>& clusterSums,
                                     const std::vector<std::vector<int>>& dist);

    // Special optimization that examines consecutive pairs for potential improvement.
    void optimizeConsecutivePairs(std::vector<std::vector<int>>& clusters,
                                  std::vector<int>& clusterSums,
                                  const std::vector<std::vector<int>>& dist);

    // Calculate the cost delta for moving a node from one cluster to another.
    int calculateMoveDelta(int node, int source, int target,
                           const std::vector<std::vector<int>>& clusters,
                           const std::vector<std::vector<int>>& dist) const;

private:
    // Input data: node names and the link cost matrix.
    std::vector<std::string> dataPointNames_;
    std::map<std::string, std::map<std::string, int>> linkCostMatrix_;

    // User-defined parameter: maximum size of each cluster.
    int C_ = 0;

    // Computed values: total number of nodes and number of clusters.
    int N_ = 0;
    int numClusters_ = 0;

    // Internally maintained: cluster assignments (maps node index to cluster index)
    // and the final clusters (each cluster containing the node names).
    std::vector<int> clusterAssignment_;
    std::vector<std::vector<std::string>> clusters_;
};

#endif // LOCAL_SEARCH_CLUSTER_H