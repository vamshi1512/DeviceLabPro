#include <optional>

#include <gtest/gtest.h>

#include "devicelab/simulator.hpp"

namespace devicelab {
namespace {

Command make_command(const CommandType type, const std::optional<double> value = std::nullopt) {
    return Command{type, value, "test"};
}

void boot_to_idle(DeviceSimulator& simulator) {
    simulator.apply_command(make_command(CommandType::PowerOn));
    simulator.advance_tick();
    simulator.advance_tick();
    simulator.advance_tick();
    ASSERT_EQ(simulator.state(), DeviceState::Idle);
}

TEST(FaultHandlingTest, CommunicationFaultEscalatesAfterGraceWindow) {
    DeviceSimulator simulator(DeviceProfile{});
    boot_to_idle(simulator);

    simulator.apply_command(make_command(CommandType::StartCycle));
    simulator.advance_tick();
    ASSERT_EQ(simulator.state(), DeviceState::Running);

    simulator.inject_fault(FaultType::CommunicationFailure, "Test injection", true);
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Degraded);

    simulator.advance_tick();
    simulator.advance_tick();
    simulator.advance_tick();
    simulator.advance_tick();

    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Faulted);
    EXPECT_TRUE(simulator.is_fault_active(FaultType::CommunicationFailure));
}

TEST(FaultHandlingTest, OverheatRecoveryRequiresAcknowledgement) {
    DeviceSimulator simulator(DeviceProfile{});
    boot_to_idle(simulator);

    simulator.apply_command(make_command(CommandType::StartCycle));
    simulator.advance_tick();
    ASSERT_EQ(simulator.state(), DeviceState::Running);

    simulator.apply_sensor_override(SensorOverride{82.0, std::nullopt, std::nullopt, std::nullopt, std::nullopt});
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Faulted);
    EXPECT_TRUE(simulator.is_fault_active(FaultType::Overheating));

    simulator.apply_sensor_override(SensorOverride{54.0, std::nullopt, std::nullopt, std::nullopt, std::nullopt});
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Faulted);
    EXPECT_FALSE(simulator.is_fault_active(FaultType::Overheating));

    simulator.apply_command(make_command(CommandType::Heartbeat));
    simulator.apply_command(make_command(CommandType::AckFault));
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Recovering);

    simulator.advance_tick();
    simulator.advance_tick();
    EXPECT_EQ(simulator.advance_tick().state, DeviceState::Running);
}

} // namespace
} // namespace devicelab
