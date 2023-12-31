#pragma once

#include <optional>

#include "config.hpp"
#include "drone.hpp"

struct DBSurfacing : DroneBehavior {
    DBSurfacing(DroneState &drone);
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const override;
    virtual void TryChange() override;
    virtual void Process() override;
};

struct DBSearching : DroneBehavior {
    DBSearching(DroneState &drone, Quadrant quadrant);
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const override;
    virtual void TryChange() override;
    virtual void Process() override;

    Quadrant currentTarget;
};

optional<Quadrant> findNextTargetForDrone(const DroneState &drone, PlayerState &state,
                                          CreatureStateSet &visibleEnemies);
optional<Quadrant> findNewTargetsByRadar(const DroneState &drone, PlayerState &state, CreatureStateSet &visibleEnemies);
Coord cleanupDirection(Coord position, Coord target, CreatureStateSet creatures);

