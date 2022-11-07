#pragma once

#include<unordered_map>

namespace TollMgmtSystem{
    enum class VehicleCategory{
        TwoWheeler,
        HatchBack,
        Sedan,
        SUV,
        Jeep,
        Van,
        Bus
    };

    std::unordered_map<VehicleCategory, std::string>vehicleCategoryToString({
        { VehicleCategory::TwoWheeler, "TwoWheeler" },
        { VehicleCategory::HatchBack, "HatchBack" },
        { VehicleCategory::Sedan, "Sedan" },
        { VehicleCategory::SUV, "SUV" },
        { VehicleCategory::Jeep, "Jeep" },
        { VehicleCategory::Van, "Van" },
        { VehicleCategory::Bus, "Bus" },
    });

    std::unordered_map<std::string, VehicleCategory>stringToVehicleCategory({
        { "TwoWheeler", VehicleCategory::TwoWheeler},
        { "HatchBack", VehicleCategory::HatchBack },
        { "Sedan", VehicleCategory::Sedan },
        { "SUV", VehicleCategory::SUV  },
        { "Jeep", VehicleCategory::Jeep },
        { "Van", VehicleCategory::Van  },
        { "Bus", VehicleCategory::Bus },
    });
}