#ifndef DATAMINER_H
#define DATAMINER_H

#include <nlohmann/json.hpp>
#include <vector>
#include <string>

class GamePlayer;  // Forward declaration of GamePlayer class
class Inventory;   // Forward declaration of Inventory class

class DataMiner {
public:
    // Constructor (Optional: You could initialize run_id_ and gameframe_ here)
    DataMiner(const std::string& run_id)
        : run_id_(run_id) {}

    // Method to process a single snapshot of GamePlayer
    void ProcessSnapshot(GamePlayer& player, uint32_t gameframe);

    // Method to flush all accumulated data to a file
    void flush(const std::string& directory);

private:
    // Internal container to store processed snapshots
    std::vector<nlohmann::json> snapshots_;

    // Member variables for run_id and gameframe
    std::string run_id_;
};

#endif // DATAMINER_H
