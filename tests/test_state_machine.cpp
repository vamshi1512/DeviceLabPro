#include <optional>

#include <gtest/gtest.h>

#include "devicelab/simulator.hpp"

namespace devicelab {
namespace {

Command make_command(const CommandType type, const std::optional<double> value = std::nullopt) {
    return Command{type, value, "test"};
}

TEST(StateMachineTest, BootRunStopAndShutdownFlow) {
    DeviceSimulator simulator(DeviceProfile{});

    simulator.apply_command(make_command(CommandType::PowerOn));
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Booting);
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Booting);
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Idle);

    simulator.apply_command(make_command(CommandType::StartCycle));
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Running);

    simulator.apply_command(make_command(CommandType::StopCycle));
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Idle);

    simulator.apply_command(make_command(CommandType::PowerOff));
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Shutdown);
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Off);
}

TEST(StateMachineTest, TargetTemperatureCommandIsSafelyClamped) {
    DeviceSimulator simulator(DeviceProfile{});

    simulator.apply_command(make_command(CommandType::SetTargetTemperature, 400.0));
    EXPECT_LT(simulator.target_temperature(), 79.0);

    simulator.apply_command(make_command(CommandType::SetTargetTemperature, 10.0));
    EXPECT_GT(simulator.target_temperature(), 24.0);
}

} // namespace
} // namespace devicelab
