/**
* This header implements a policy that maps new applications to the coldest
* core and migrates the task on the hottest core to the coldest core in some
* timeslices.
*/
#ifndef __GLOBAL_POWER_MULTIPLEXING_H
#define __GLOBAL_POWER_MULTIPLEXING_H
#include <vector>

#include "mappingpolicy.h"
#include "migrationpolicy.h"
#include "performance_counters.h"

class GlobalPowerMultiplexing : public MappingPolicy, public MigrationPolicy {
   public:
    GlobalPowerMultiplexing(const PerformanceCounters *performanceCounters, int coreRows,
                            int coreColumns, float criticalTemperature, SubsecondTime migrationInterval);
    virtual std::vector<int> map(String taskName, int taskCoreRequirement,
                                 const std::vector<bool> &availableCores,
                                 const std::vector<bool> &activeCores);
    virtual std::vector<migration> migrate(
        SubsecondTime time, const std::vector<int> &taskIds,
        const std::vector<bool> &activeCores);

   private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    float criticalTemperature;
    SubsecondTime migrationInterval;
    SubsecondTime lastMigrationTime;

    int getColdestCore(const std::vector<bool> &availableCores);
    int getHottestCore(const std::vector<bool> &activeCores);
    void logTemperatures(const std::vector<bool> &availableCores);
};
#endif
