#include <filesystem>
#include <fstream>
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

TEST(ValidationTest, RejectsInvalidDeviceProfile) {
    DeviceProfile profile;
    profile.id.clear();
    EXPECT_THROW(validate_device_profile(profile), std::runtime_error);
}

TEST(ValidationTest, RejectsScenarioEventsOutsideDuration) {
    ScenarioDefinition scenario;
    scenario.id = "invalid";
    scenario.name = "Invalid";
    scenario.duration_ticks = 5;
    scenario.events.push_back(ScenarioEvent{
        .tick = 9,
        .type = ScenarioEventType::Note,
        .note = "late event",
    });

    EXPECT_THROW(validate_scenario_definition(scenario), std::runtime_error);
}

TEST(ValidationTest, ReplayFailsWhenManifestDigestIsTampered) {
    const auto root = source_root();
    const auto scenario = load_scenario_definition(root / "scenarios/nominal_behavior.json");
    const auto profile = load_device_profile(root / "profiles/thermal_controller_mk1.json");
    const auto first_output = temp_run_dir("tampered-first");
    const auto second_output = temp_run_dir("tampered-replay");

    const auto initial = run_scenario(scenario, profile, first_output);
    std::ifstream input(first_output / "replay_manifest.json");
    auto manifest = json::parse(input);
    manifest["expected_digest"] = "deadbeefdeadbeef";

    {
        std::ofstream output(first_output / "replay_manifest.json");
        output << manifest.dump(2) << '\n';
    }

    const auto replay = replay_run(first_output / "replay_manifest.json", second_output);

    EXPECT_TRUE(initial.passed);
    EXPECT_FALSE(replay.passed);
    EXPECT_NE(initial.digest, manifest["expected_digest"].get<std::string>());
}

} // namespace
} // namespace devicelab
