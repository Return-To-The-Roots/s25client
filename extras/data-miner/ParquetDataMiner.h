
#ifndef PARQUET_DATA_MINER_H
#define PARQUET_DATA_MINER_H

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class GamePlayer;

class ParquetDataMiner {
public:
    /**
     * @brief Construct a new Parquet Data Miner object
     *
     * @param output_dir Directory where Parquet files will be saved
     * @param run_id Unique identifier for this mining session
     */
    ParquetDataMiner(const std::string& output_dir, const std::string& run_id);

    ~ParquetDataMiner();

    /**
     * @brief Records all relevant data for a game frame
     *
     * @param player The player to mine data from
     * @param gf Current game frame number
     */
    void RecordFrameData(GamePlayer& player, unsigned gf);

private:
    // Initializes Arrow schemas
    void InitSchemas();

    // Records high-level game statistics
    void RecordMacroStats(GamePlayer& player);

    // Records building-specific data
    void RecordBuildingStats(GamePlayer& player);

    // Writes all buffered data to disk
    void FlushAll();

    /**
     * @brief Writes record batches to a Parquet file
     *
     * @param batches Vector of record batches to write
     * @param schema Schema for the data
     * @param filename Output filename (without path)
     */
    void WriteBatchesToFile(
        const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
        const std::shared_ptr<arrow::Schema>& schema,
        const std::string& filename);

    std::string run_id_;
    std::string output_dir_;
    unsigned gf_ = 0;
    size_t batch_size_ = 1000; // Flush after this many records

    // Arrow schemas
    std::shared_ptr<arrow::Schema> macro_schema_;
    std::shared_ptr<arrow::Schema> building_schema_;
    std::shared_ptr<arrow::Schema> goods_schema_; // For future use

    // Data buffers
    std::vector<std::shared_ptr<arrow::RecordBatch>> macro_batches_;
    std::vector<std::shared_ptr<arrow::RecordBatch>> building_batches_;
};

#endif // PARQUET_DATA_MINER_H