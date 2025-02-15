#ifndef SLIDING_WINDOW_HPP
#define SLIDING_WINDOW_HPP

#include <deque>
#include <numeric>
#include <iostream>
#include <ns3/simulator.h>

namespace nfd {
namespace utils {

template <typename T>
class SlidingWindow {
public:
    struct DataInfo {
        ns3::Time arrivalTime;  // Arrival time of the data
        T value;           // Value associated with the data, expected as qsf; or local data queue size in updated design
    };

    SlidingWindow() : m_windowDuration(ns3::MilliSeconds(10)) {}

    SlidingWindow(ns3::Time windowDuration) : m_windowDuration(windowDuration) {}

    void AddPacket(ns3::Time newTime, T value) {
        m_data.push_back({newTime, value});

        // Remove outdated packets
        while (!m_data.empty() && (newTime - m_data.front().arrivalTime) > m_windowDuration) {
            m_data.pop_front();
        }
    }

    size_t GetCurrentWindowSize() const {
        return m_data.size();
    }

    double GetAverageQueue() const {
        if (m_data.empty()) return 0.0;

        double sum = std::accumulate(m_data.begin(), m_data.end(), 0.0, [](double acc, const DataInfo& info) {
            return acc + info.value;
        });
        return sum / m_data.size();
    }

    // Unit - pkgs/us
    double GetDataArrivalRate() const {
        if (m_data.size() < 2) {
            return 0.0;
        }

        //* Get time by microseconds, finally transfer rate back into pkgs/ms
        double duration = (m_data.back().arrivalTime - m_data.front().arrivalTime).GetNanoSeconds();
        if (duration <= 0) {
            return -1.0;
        }
        return static_cast<double>((m_data.size() - 1)) / duration * 1e3;
    }

/*     // Return data rate with error handling
    double GetDataRate() const {
        double rawDataRate = GetDataArrivalRate();

        if (rawDataRate == -1) {
            LogWarning("Returned data arrival rate is -1, please check!");
            return 0.0;  
        } else if (rawDataRate == 0) {
            LogInfo("Sliding window is not enough, use 0 as data arrival rate: 0 pkgs/ms");
            return 0.0;
        } else {
            return rawDataRate;
        }
    } */

private:
    ns3::Time m_windowDuration;
    std::deque<DataInfo> m_data;

/*     // Logging helper functions
    void LogInfo(const std::string& message) const {
        std::cout << "[INFO] SlidingWindow: " << message << std::endl;
    }

    void LogWarning(const std::string& message) const {
        std::cerr << "[WARNING] SlidingWindow: " << message << std::endl;
    } */
};

} // namespace utils
} // namespace nfd

#endif // SLIDING_WINDOW_HPP