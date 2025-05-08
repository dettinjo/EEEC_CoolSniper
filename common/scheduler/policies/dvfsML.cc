#include "dvfsML.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

DVFSML::DVFSML(const PerformanceCounters *performanceCounters,
               int coreRows, int coreColumns, int minFrequency,
               int maxFrequency, int frequencyStepSize,
               float upThreshold, float downThreshold,
               float dtmCriticalTemperature,
               float dtmRecoveredTemperature)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      minFrequency(minFrequency),
      maxFrequency(maxFrequency),
      frequencyStepSize(frequencyStepSize),
      upThreshold(upThreshold),
      downThreshold(downThreshold),
      dtmCriticalTemperature(dtmCriticalTemperature),
      dtmRecoveredTemperature(dtmRecoveredTemperature) {
    prevTemperatures.resize(coreRows * coreColumns, 0.0f);
    
    // Open CSV file for writing
    csvFile.open("dvfs_ml_data.csv");
    if (csvFile.is_open()) {
        csvFile << "Core,Power,Frequency,Temperature,Utilization,InThrottleMode" << endl;
    } else {
        cerr << "Error: Unable to open CSV file for writing." << endl;
    }
}

std::vector<int> DVFSML::getFrequencies(
    const std::vector<int> &oldFrequencies,
    const std::vector<bool> &activeCores) {
    MLData mlData = collectMLData(oldFrequencies);
    
    if (throttle()) {
        std::vector<int> minFrequencies(coreRows * coreColumns, minFrequency);
        cout << "[Scheduler][ML-DTM]: in throttle mode -> return min. frequencies " << endl;
        
        // Collect data even in throttle mode
        for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
            float power = mlData.currPowers[coreCounter];
            float temperature = mlData.currTemperatures[coreCounter];
            float utilization = performanceCounters->getUtilizationOfCore(coreCounter);
            
            if (csvFile.is_open()) {
                csvFile << coreCounter << ","
                        << power << ","
                        << minFrequency << ","
                        << temperature << ","
                        << utilization << ","
                        << "1" << endl;  // 1 indicates in throttle mode
            }
        }
        
        return minFrequencies;
    } else {
        std::vector<int> frequencies(coreRows * coreColumns);
        for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
             coreCounter++) {
            if (activeCores.at(coreCounter)) {
                float power = mlData.currPowers[coreCounter];
                float temperature = mlData.currTemperatures[coreCounter];
                int frequency = oldFrequencies.at(coreCounter);
                float utilization =
                    performanceCounters->getUtilizationOfCore(coreCounter);
                cout << "[Scheduler][ML]: Core " << setw(2)
                     << coreCounter << ":";
                cout << " P=" << fixed << setprecision(3) << power << " W";
                cout << " f=" << frequency << " MHz";
                cout << " T=" << fixed << setprecision(1) << temperature
                     << " C";
                cout << " utilization=" << fixed << setprecision(3)
                     << utilization << endl;
                
                // Save data to CSV file
                if (csvFile.is_open()) {
                    csvFile << coreCounter << ","
                            << power << ","
                            << frequency << ","
                            << temperature << ","
                            << utilization << ","
                            << "0" << endl;  // 0 indicates not in throttle mode
                }
                
                // ML-based decision logic would go here
                // For now, we'll use a simplified version of the ondemand logic
                if (utilization > upThreshold) {
                    cout << "[Scheduler][ML]: utilization > upThreshold";
                    if (frequency == maxFrequency) {
                        cout << " but already at max frequency" << endl;
                    } else {
                        cout << " -> go to max frequency" << endl;
                        frequency = maxFrequency;
                    }
                } else if (utilization < downThreshold) {
                    cout << "[Scheduler][ML]: utilization < downThreshold";
                    if (frequency == minFrequency) {
                        cout << " but already at min frequency" << endl;
                    } else {
                        cout << " -> lower frequency" << endl;
                        frequency = frequency * 80 / 100;
                        frequency = (frequency / frequencyStepSize) *
                                    frequencyStepSize;  // round
                        if (frequency < minFrequency) {
                            frequency = minFrequency;
                        }
                    }
                }
                frequencies.at(coreCounter) = frequency;
            } else {
                frequencies.at(coreCounter) = minFrequency;
            }
        }
        return frequencies;
    }
}

bool DVFSML::throttle() {
    if (performanceCounters->getPeakTemperature() > dtmCriticalTemperature) {
        if (!in_throttle_mode) {
            cout << "[Scheduler][ML-DTM]: detected thermal violation"
                 << endl;
        }
        in_throttle_mode = true;
    } else if (performanceCounters->getPeakTemperature() <
               dtmRecoveredTemperature) {
        if (in_throttle_mode) {
            cout << "[Scheduler][ML-DTM]: thermal violation ended"
                 << endl;
        }
        in_throttle_mode = false;
    }
    return in_throttle_mode;
}

MLData DVFSML::collectMLData(const std::vector<int> &oldFrequencies) {
    MLData data;
    data.prevTemperatures = prevTemperatures;
    data.currTemperatures.resize(coreRows * coreColumns);
    data.currFrequencies = oldFrequencies;
    data.currPowers.resize(coreRows * coreColumns);
    data.inThrottleMode = in_throttle_mode;

    for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
        data.currTemperatures[coreCounter] = performanceCounters->getTemperatureOfCore(coreCounter);
        data.currPowers[coreCounter] = performanceCounters->getPowerOfCore(coreCounter);
    }

    prevTemperatures = data.currTemperatures;
    return data;
}

DVFSML::~DVFSML() {
    if (csvFile.is_open()) {
        csvFile.close();
    }
}
