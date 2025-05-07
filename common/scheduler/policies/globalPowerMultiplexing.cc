#include "globalPowerMultiplexing.h"

#include <iomanip>
#include <limits>
using namespace std;

GlobalPowerMultiplexing::GlobalPowerMultiplexing(const PerformanceCounters *performanceCounters,
                         int coreRows, int coreColumns,
                         float criticalTemperature, SubsecondTime migrationInterval)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      criticalTemperature(criticalTemperature),
      migrationInterval(migrationInterval),
      lastMigrationTime(SubsecondTime::Zero()) {}

std::vector<int> GlobalPowerMultiplexing::map(String taskName, int taskCoreRequirement,
                                  const std::vector<bool> &availableCoresRO,
                                  const std::vector<bool> &activeCores) {
    std::vector<bool> availableCores(availableCoresRO);
    std::vector<int> cores;
    logTemperatures(availableCores);
    for (; taskCoreRequirement > 0; taskCoreRequirement--) {
        int coldestCore = getColdestCore(availableCores);
        if (coldestCore == -1) {
            // not enough free cores
            std::vector<int> empty;
            return empty;
        } else {
            cores.push_back(coldestCore);
            availableCores.at(coldestCore) = false;
        }
    }
    return cores;
}

std::vector<migration> GlobalPowerMultiplexing::migrate(
    SubsecondTime time, const std::vector<int> &taskIds,
    const std::vector<bool> &activeCores) {

    cout << "[Scheduler][GlobalPowerMultiplexing-migrate]: time: " << time  << " ns" << endl;
    cout << "[Scheduler][GlobalPowerMultiplexing-migrate]: lastMigrationTime: " << lastMigrationTime  << " ns" << endl;
    cout << "[Scheduler][GlobalPowerMultiplexing-migrate]: migrationInterval: " << migrationInterval  << " ns" << endl;

    std::vector<migration> migrations;

    if (time - lastMigrationTime >= migrationInterval) {
        int hottestCore = getHottestCore(activeCores);
        std::vector<bool> availableCores(coreRows * coreColumns, false);
        for (int c = 0; c < coreRows * coreColumns; c++) {
            if (taskIds[c] == -1 && c != hottestCore) {
                availableCores[c] = true;
            }
        }

        int coldestCore = getColdestCore(availableCores);

        if (hottestCore != -1 && coldestCore != -1) {
            migration m;
            m.fromCore = hottestCore;
            m.toCore = coldestCore;
            m.swap = false;
            migrations.push_back(m);

            cout << "[Scheduler][GlobalPowerMultiplexing-migrate]: Migrating from core " 
                 << hottestCore << " (temp: " << fixed << setprecision(1) 
                 << performanceCounters->getTemperatureOfCore(hottestCore)
                 << ") to core " << coldestCore << " (temp: " 
                 << performanceCounters->getTemperatureOfCore(coldestCore) << ")" << endl;
        }

        lastMigrationTime = time;
    }

    return migrations;
}

int GlobalPowerMultiplexing::getColdestCore(const std::vector<bool> &availableCores) {
    int coldestCore = -1;
    float coldestTemperature = std::numeric_limits<float>::max();
    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (availableCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if (temperature < coldestTemperature) {
                coldestCore = c;
                coldestTemperature = temperature;
            }
        }
    }
    return coldestCore;
}

int GlobalPowerMultiplexing::getHottestCore(const std::vector<bool> &activeCores) {
    int hottestCore = -1;
    float hottestTemperature = -1;
    for (int c = 0; c < coreRows * coreColumns; c++) {
        if (activeCores.at(c)) {
            float temperature = performanceCounters->getTemperatureOfCore(c);
            if (temperature > hottestTemperature) {
                hottestCore = c;
                hottestTemperature = temperature;
            }
        }
    }
    return hottestCore;
}

void GlobalPowerMultiplexing::logTemperatures(const std::vector<bool> &availableCores) {
    cout << "[Scheduler][GlobalPowerMultiplexing-map]: temperatures of available cores:"
         << endl;
    for (int y = 0; y < coreRows; y++) {
        for (int x = 0; x < coreColumns; x++) {
            if (x > 0) {
                cout << " ";
            }
            int coreId = y * coreColumns + x;
            if (!availableCores.at(coreId)) {
                cout << " - ";
            } else {
                float temperature =
                    performanceCounters->getTemperatureOfCore(coreId);
                cout << fixed << setprecision(1) << temperature;
            }
        }
        cout << endl;
    }
}
