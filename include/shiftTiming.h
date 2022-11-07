#pragma once

namespace TollMgmtSystem{
    enum class ShiftTiming{
        Day,
        Night,
    };

    std::unordered_map<ShiftTiming, std::string>shiftTimingToString({
        { ShiftTiming::Day, "Day" },
        { ShiftTiming::Night, "Night" },
    });

    std::unordered_map<std::string, ShiftTiming>stringToShiftTiming({
        { "Day", ShiftTiming::Day  },
        { "Night", ShiftTiming::Night  },
    });
}