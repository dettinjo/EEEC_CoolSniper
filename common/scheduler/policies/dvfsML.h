/**
 * This header implements the ML-based DVFS governor with DTM.
 */
#ifndef __DVFS_ML_H
#define __DVFS_ML_H
#include <vector>
#include <fstream>

#include "dvfspolicy.h"
#include "performance_counters.h"

struct MLData {
    std::vector<float> prevTemperatures;
    std::vector<float> currTemperatures;
    std::vector<int> currFrequencies;
    std::vector<float> currPowers;
    std::vector<float> currUtilizations;
    std::vector<float> prevPowers;
    std::vector<float> prevUtilizations;
    bool inThrottleMode;
};

class DVFSML : public DVFSPolicy {
   public:
    DVFSML(const PerformanceCounters *performanceCounters, int coreRows,
           int coreColumns, int minFrequency, int maxFrequency,
           int frequencyStepSize, float upThreshold, float downThreshold,
           float dtmCriticalTemperature, float dtmRecoveredTemperature);
    virtual std::vector<int> getFrequencies(
        const std::vector<int> &oldFrequencies,
        const std::vector<bool> &activeCores);
    virtual ~DVFSML();

   private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;
    int frequencyStepSize;
    float upThreshold;
    float downThreshold;
    float dtmCriticalTemperature;
    float dtmRecoveredTemperature;
    bool in_throttle_mode = false;
    bool throttle();
    MLData collectMLData(const std::vector<int> &oldFrequencies);
    std::vector<float> prevTemperatures;
    std::vector<float> prevPowers;
    std::vector<float> prevUtilizations;
    std::ofstream csvFile;
    static double overheadTime; // New field to save overhead time
};
#endif