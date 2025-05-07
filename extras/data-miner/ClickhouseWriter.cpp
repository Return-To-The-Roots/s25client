#include <clickhouse/client.h>
#include <clickhouse/columns/array.h>
#include <clickhouse/columns/string.h>
#include <clickhouse/columns/numeric.h>
#include <clickhouse/columns/map.h>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

class ClickhouseWriter {
private:
    std::string database_name_;
    std::string table_name_;
    clickhouse::Client client_;

public:
    ClickhouseWriter(const std::string& host,
                    uint16_t port,
                    const std::string& database,
                    const std::string& table,
                    const std::string& username,
                    const std::string& password)
        : database_name_(database),
          table_name_(table),
          client_(GetClientOptions(host, port, username, password))
{
    // Constructor body is now empty since initialization is done in the initializer list
}

// Helper method to create client options
clickhouse::ClientOptions GetClientOptions(
    const std::string& host,
    uint16_t port,
    const std::string& username,
    const std::string& password) {

    clickhouse::ClientOptions options;
    options.SetHost(host);
    options.SetPort(port);

    if (!username.empty()) {
        options.SetUser(username);
        if (!password.empty()) {
            options.SetPassword(password);
        }
    }

    return options;
}

    void WriteGameData(const std::string& run_id,
                       uint32_t gameframe,
                       const std::vector<std::pair<std::string, uint32_t>>& statistics,
                       const std::vector<std::tuple<std::string, uint16_t, uint16_t>>& buildings,
                       const std::map<std::string, uint32_t>& merchandise) {

        try {
            // Create columns for the table
            auto run_id_col = std::make_shared<clickhouse::ColumnString>();
            auto gameframe_col = std::make_shared<clickhouse::ColumnUInt32>();

            // Statistics nested columns
            auto stat_types_col = std::make_shared<clickhouse::ColumnString>();
            auto stat_values_col = std::make_shared<clickhouse::ColumnUInt32>();

            // Buildings nested columns
            auto building_types_col = std::make_shared<clickhouse::ColumnString>();
            auto building_counts_col = std::make_shared<clickhouse::ColumnUInt16>();
            auto building_sites_col = std::make_shared<clickhouse::ColumnUInt16>();

            // Merchandise map column
            auto merch_keys_col = std::make_shared<clickhouse::ColumnString>();
            auto merch_values_col = std::make_shared<clickhouse::ColumnUInt32>();

            // Fill basic columns
            run_id_col->Append(run_id);
            gameframe_col->Append(gameframe);

            // Fill statistics nested columns
            for (const auto& stat : statistics) {
                stat_types_col->Append(stat.first);
                stat_values_col->Append(stat.second);
            }

            // Fill buildings nested columns
            for (const auto& building : buildings) {
                building_types_col->Append(std::get<0>(building));
                building_counts_col->Append(std::get<1>(building));
                building_sites_col->Append(std::get<2>(building));
            }

            // Fill merchandise map columns
            for (const auto& item : merchandise) {
                merch_keys_col->Append(item.first);
                merch_values_col->Append(item.second);
            }

            // Create array columns for nested structures
            auto stat_types_array = std::make_shared<clickhouse::ColumnArray>(
                stat_types_col,
                std::make_shared<clickhouse::ColumnUInt64>(std::vector<uint64_t>{statistics.size()})
            );

            auto stat_values_array = std::make_shared<clickhouse::ColumnArray>(
                stat_values_col,
                std::make_shared<clickhouse::ColumnUInt64>(std::vector<uint64_t>{statistics.size()})
            );

            auto building_types_array = std::make_shared<clickhouse::ColumnArray>(
                building_types_col,
                std::make_shared<clickhouse::ColumnUInt64>(std::vector<uint64_t>{buildings.size()})
            );

            auto building_counts_array = std::make_shared<clickhouse::ColumnArray>(
                building_counts_col,
                std::make_shared<clickhouse::ColumnUInt64>(std::vector<uint64_t>{buildings.size()})
            );

            auto building_sites_array = std::make_shared<clickhouse::ColumnArray>(
                building_sites_col,
                std::make_shared<clickhouse::ColumnUInt64>(std::vector<uint64_t>{buildings.size()})
            );

            // Create map column
            auto merchandise_map = std::make_shared<clickhouse::ColumnMap>(
                merch_keys_col,
                merch_values_col
            );

            // Assemble the block
            clickhouse::Block block;
            block.AppendColumn("run_id", run_id_col);
            block.AppendColumn("gameframe", gameframe_col);
            block.AppendColumn("statistics.type", stat_types_array);
            block.AppendColumn("statistics.value", stat_values_array);
            block.AppendColumn("buildings.type", building_types_array);
            block.AppendColumn("buildings.count", building_counts_array);
            block.AppendColumn("buildings.sites", building_sites_array);
            block.AppendColumn("merchandise", merchandise_map);

            // Insert data
            client_.Insert(database_name_ + "." + table_name_, block);

        } catch (const std::exception& e) {
            std::cerr << "Error writing to ClickHouse: " << e.what() << std::endl;
        }
    }
};
