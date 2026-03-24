#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "devicelab/simulator.hpp"

namespace devicelab {
namespace {

std::filesystem::path source_root() {
    return std::filesystem::path(DEVICELAB_SOURCE_DIR);
}

std::filesystem::path temp_run_dir(const std::string& name) {
    return std::filesystem::temp_directory_path() / "devicelab_pro_tests" / name;
}

TEST(ScenarioRunnerTest, NominalScenarioPassesAndWritesArtifacts) {
    const auto root = source_root();
    const auto scenario = load_scenario_definition(root / "scenarios/nominal_behavior.json");
    const auto profile = load_device_profile(root / "profiles/thermal_controller_mk1.json");
    const auto output_dir = temp_run_dir("nominal");

    const auto artifacts = run_scenario(scenario, profile, output_dir);

    EXPECT_TRUE(artifacts.passed);
    EXPECT_EQ(artifacts.telemetry.size(), scenario.duration_ticks);
    EXPECT_TRUE(std::filesystem::exists(output_dir / "summary.json"));
    EXPECT_TRUE(std::filesystem::exists(output_dir / "telemetry.jsonl"));
    EXPECT_TRUE(std::filesystem::exists(output_dir / "replay_manifest.json"));
}

TEST(ScenarioRunnerTest, ReplayProducesIdenticalDigest) {
    const auto root = source_root();
    const auto scenario = load_scenario_definition(root / "scenarios/degraded_behavior.json");
    const auto profile = load_device_profile(root / "profiles/thermal_controller_mk1.json");
    const auto first_output = temp_run_dir("degraded-first");
    const auto replay_output = temp_run_dir("degraded-replay");

    const auto first_run = run_scenario(scenario, profile, first_output);
    const auto replay = replay_run(first_output / "replay_manifest.json", replay_output);

    EXPECT_TRUE(first_run.passed);
    EXPECT_TRUE(replay.passed);
    EXPECT_EQ(first_run.digest, replay.digest);
}

} // namespace
} // namespace devicelab
