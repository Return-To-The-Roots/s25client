#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/status.h>
#include <arrow/util/macros.h>
#include <arrow/compute/api.h>
#include <arrow/compute/kernel.h>
#include <parquet/arrow/writer.h>
#include "Game.h"
#include "GamePlayer.h"
#include "PlayerInfo.h"
#include "gameTypes/StatisticTypes.h"
#include "ParquetDataMiner.h"

#define ARROW_CHECK_OK(status) \
    do { \
        arrow::Status _s = (status); \
        if (!_s.ok()) { \
            std::cerr << "Arrow error at " << __FILE__ << ":" << __LINE__ << ": " \
                      << _s.ToString() << std::endl; \
            std::abort(); \
        } \
    } while (0)

ParquetDataMiner::ParquetDataMiner(const std::string& output_dir, const std::string& run_id)
    : run_id_(run_id), output_dir_(output_dir) {
    // Initialize schemas
    InitSchemas();
}

ParquetDataMiner::~ParquetDataMiner() {
    // Flush any remaining data when destroyed
    FlushAll();
}

void ParquetDataMiner::InitSchemas() {
    // Macro statistics schema
    std::vector<std::shared_ptr<arrow::Field>> macro_fields = {
        arrow::field("run_id", arrow::utf8()),
        arrow::field("gameframe", arrow::uint32()),
        arrow::field("country_size", arrow::uint32()),
        arrow::field("total_buildings", arrow::uint32()),
        arrow::field("military_strength", arrow::uint32()),
        arrow::field("gold_coins", arrow::uint32()),
        arrow::field("productivity", arrow::uint32()),
        arrow::field("kills", arrow::uint32())
    };
    macro_schema_ = arrow::schema(macro_fields);

    // Buildings schema
    std::vector<std::shared_ptr<arrow::Field>> building_fields = {
        arrow::field("run_id", arrow::utf8()),
        arrow::field("gameframe", arrow::uint32()),
        arrow::field("building_type", arrow::utf8()),
        arrow::field("count", arrow::uint32()),
        arrow::field("sites", arrow::uint32()),
        arrow::field("productivity", arrow::uint8())
    };
    building_schema_ = arrow::schema(building_fields);
}

void ParquetDataMiner::RecordFrameData(GamePlayer& player, unsigned gf) {
    gf_ = gf;

    // Record macro statistics
    RecordMacroStats(player);

    // Record building data
    RecordBuildingStats(player);

    // Periodically flush data to disk
    if (macro_batches_.size() >= batch_size_ || building_batches_.size() >= batch_size_) {
        FlushAll();
    }
}
void ParquetDataMiner::PrintData(GamePlayer& player)
{
    std::cout <<  " gameframe: " << gf_ << std::endl;
    std::cout <<  " run_id: " << run_id_ << std::endl;
    // Macro statistics
    for (StatisticType type : helpers::EnumRange<StatisticType>{})
    {
        uint32_t value = player.GetStatisticCurrentValue(type);
        std::cout << StatisticTypeName(type) << value << std::endl;
    }
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();

    // buildings data
    for (BuildingType type : helpers::EnumRange<BuildingType>{})
    {
        unsigned count = building_nums.buildings[type];
        unsigned sites = building_nums.buildings[type];
        std::cout << BUILDING_NAMES_1.at(type) << " count: " << count << " sites: " << sites << std::endl;
    }

    // Merchandise
    Inventory inventory = player.GetInventory();
    for (GoodType type : helpers::EnumRange<GoodType>{})
    {
        unsigned count = inventory.goods[type];
        std::cout << GOOD_NAMES_1.at(type) << " count: " << std::endl;
    }
}

void ParquetDataMiner::RecordMacroStats(GamePlayer& player) {
    arrow::UInt32Builder gameframe_builder;
    arrow::StringBuilder run_id_builder;
    arrow::UInt32Builder country_builder, buildings_builder, military_builder;
    arrow::UInt32Builder coins_builder, productivity_builder, kills_builder;

    ARROW_CHECK_OK(gameframe_builder.Append(gf_));
    ARROW_CHECK_OK(run_id_builder.Append(run_id_));

    // Assuming StatisticType enum has these in order
    for (StatisticType type : helpers::EnumRange<StatisticType>{}) {
        uint32_t value = player.GetStatisticCurrentValue(type);

        switch (type) {
            case StatisticType::Country: ARROW_CHECK_OK(country_builder.Append(value)); break;
            case StatisticType::Buildings: ARROW_CHECK_OK(buildings_builder.Append(value)); break;
            case StatisticType::Military: ARROW_CHECK_OK(military_builder.Append(value)); break;
            case StatisticType::Gold: ARROW_CHECK_OK(coins_builder.Append(value)); break;
            case StatisticType::Productivity: ARROW_CHECK_OK(productivity_builder.Append(value)); break;
            case StatisticType::Vanquished: ARROW_CHECK_OK(kills_builder.Append(value)); break;
            default: break;
        }
    }

    std::shared_ptr<arrow::Array> gameframe_arr, run_id_arr, country_arr, buildings_arr;
    std::shared_ptr<arrow::Array> military_arr, coins_arr, productivity_arr, kills_arr;

    ARROW_CHECK_OK(gameframe_builder.Finish(&gameframe_arr));
    ARROW_CHECK_OK(run_id_builder.Finish(&run_id_arr));
    ARROW_CHECK_OK(country_builder.Finish(&country_arr));
    ARROW_CHECK_OK(buildings_builder.Finish(&buildings_arr));
    ARROW_CHECK_OK(military_builder.Finish(&military_arr));
    ARROW_CHECK_OK(coins_builder.Finish(&coins_arr));
    ARROW_CHECK_OK(productivity_builder.Finish(&productivity_arr));
    ARROW_CHECK_OK(kills_builder.Finish(&kills_arr));

    auto batch = arrow::RecordBatch::Make(macro_schema_, 1, {
        run_id_arr, gameframe_arr, country_arr, buildings_arr,
        military_arr, coins_arr, productivity_arr, kills_arr
    });

    macro_batches_.push_back(batch);
}

void ParquetDataMiner::RecordBuildingStats(GamePlayer& player) {
    const auto& building_nums = player.GetBuildingRegister().GetBuildingNums();

    for (BuildingType type : helpers::EnumRange<BuildingType>{}) {
        arrow::StringBuilder run_id_builder;
        arrow::UInt32Builder gameframe_builder;
        arrow::StringBuilder type_builder;
        arrow::UInt32Builder count_builder, sites_builder;
        arrow::UInt8Builder productivity_builder;

        ARROW_CHECK_OK(run_id_builder.Append(run_id_));
        ARROW_CHECK_OK(gameframe_builder.Append(gf_));
        ARROW_CHECK_OK(type_builder.Append(BUILDING_NAMES_1.at(type)));
        ARROW_CHECK_OK(count_builder.Append(building_nums.buildings[type]));
        ARROW_CHECK_OK(sites_builder.Append(building_nums.buildingSites[type]));

        // TODO: Add productivity if available
        ARROW_CHECK_OK(productivity_builder.Append(0)); // Placeholder

        std::shared_ptr<arrow::Array> run_id_arr, gameframe_arr, type_arr;
        std::shared_ptr<arrow::Array> count_arr, sites_arr, productivity_arr;

        ARROW_CHECK_OK(run_id_builder.Finish(&run_id_arr));
        ARROW_CHECK_OK(gameframe_builder.Finish(&gameframe_arr));
        ARROW_CHECK_OK(type_builder.Finish(&type_arr));
        ARROW_CHECK_OK(count_builder.Finish(&count_arr));
        ARROW_CHECK_OK(sites_builder.Finish(&sites_arr));
        ARROW_CHECK_OK(productivity_builder.Finish(&productivity_arr));

        auto batch = arrow::RecordBatch::Make(building_schema_, 1, {
            run_id_arr, gameframe_arr, type_arr,
            count_arr, sites_arr, productivity_arr
        });

        building_batches_.push_back(batch);
    }
}

void ParquetDataMiner::FlushAll() {
    std::cout << "Flushing" << std::endl;

    if (!macro_batches_.empty()) {
        WriteBatchesToFile(macro_batches_, macro_schema_, "macro_stats.parquet");
        macro_batches_.clear();
    }

    if (!building_batches_.empty()) {
        WriteBatchesToFile(building_batches_, building_schema_, "buildings.parquet");
        building_batches_.clear();
    }
}

void ParquetDataMiner::WriteBatchesToFile(
    const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
    const std::shared_ptr<arrow::Schema>& schema,
    const std::string& filename) {

    // 1. Create unsorted table
    arrow::Result<std::shared_ptr<arrow::Table>> table_result =
        arrow::Table::FromRecordBatches(schema, batches);
    if (!table_result.ok()) {
        std::cerr << "Table creation failed: " << table_result.status().ToString() << std::endl;
        return;
    }

    // 2. Sort by gameframe (ascending)
    arrow::compute::SortOptions sort_options({
        arrow::compute::SortKey("gameframe", arrow::compute::SortOrder::Ascending)
    });

    arrow::Result<arrow::Datum> sort_result =
        arrow::compute::SortIndices(table_result.ValueOrDie(), sort_options);
    if (!sort_result.ok()) {
        std::cerr << "Sort failed: " << sort_result.status().ToString() << std::endl;
        return;
    }

    // 3. Apply sorting
    arrow::Result<arrow::Datum> take_result =
        arrow::compute::Take(table_result.ValueOrDie(), sort_result.ValueOrDie());
    if (!take_result.ok()) {
        std::cerr << "Take failed: " << take_result.status().ToString() << std::endl;
        return;
    }

    std::shared_ptr<arrow::Table> sorted_table =
        take_result.ValueOrDie().table();

    // 4. Create output file
    arrow::Result<std::shared_ptr<arrow::io::FileOutputStream>> outfile_result =
        arrow::io::FileOutputStream::Open(output_dir_ + "/" + filename);

    if (!outfile_result.ok()) {
        std::cerr << "File open failed: " << outfile_result.status().ToString() << std::endl;
        return;
    }

    // 5. Write to Parquet
    arrow::Status write_status = parquet::arrow::WriteTable(
        *sorted_table,
        arrow::default_memory_pool(),
        *outfile_result,
        65536);

    if (!write_status.ok()) {
        std::cerr << "Parquet write failed: " << write_status.ToString() << std::endl;
    }
}