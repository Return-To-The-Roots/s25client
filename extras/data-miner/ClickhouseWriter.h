#pragma once

#include <clickhouse/client.h>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <memory>

/**
 * @brief A class to handle writing game data to ClickHouse database
 *
 * This class provides functionality to connect to a ClickHouse server
 * and insert game snapshot data into the specified table.
 */
class ClickhouseWriter {
    /**
     * @brief Write game data to ClickHouse table
     *
     * @param run_id Unique identifier for the game run
     * @param gameframe Current game frame
     * @param statistics Vector of statistic name and value pairs
     * @param buildings Vector of tuples containing building type, count, and sites
     * @param merchandise Map of merchandise type to count
     */

public:
    /**
     * @brief Construct a new ClickhouseWriter object
     *
     * @param host ClickHouse server host
     * @param port ClickHouse server port
     * @param database Database name
     * @param table Table name
     * @param username Optional username for authentication
     * @param password Optional password for authentication
     */
    ClickhouseWriter(const std::string& host,
                    uint16_t port,
                    const std::string& database,
                    const std::string& table,
                    const std::string& username = "",
                    const std::string& password = "");

    /**
     * @brief Helper method to create ClickHouse client options
     *
     * @param host ClickHouse server host
     * @param port ClickHouse server port
     * @param username Username for authentication
     * @param password Password for authentication
     * @return clickhouse::ClientOptions Configured options object
     */
    clickhouse::ClientOptions GetClientOptions(
        const std::string& host,
        uint16_t port,
        const std::string& username,
        const std::string& password);
    void WriteGameData(const std::string& run_id, 
                      uint32_t gameframe,
                      const std::vector<std::pair<std::string, uint32_t>>& statistics,
                      const std::vector<std::tuple<std::string, uint16_t, uint16_t>>& buildings,
                      const std::map<std::string, uint32_t>& merchandise);
};

/**
 * @brief Extension to DataMiner class to write game data to ClickHouse
 */
class DataMiner {
    // Existing DataMiner members...
    
public:
    // Existing DataMiner methods...
    
    /**
     * @brief Write game snapshot data to ClickHouse
     * 
     * Collects game data from the player object and writes it to the
     * configured ClickHouse table.
     * 
     * @param player Reference to the game player object
     */
    void WriteToClickhouse(GamePlayer& player);
};