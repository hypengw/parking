#include <gtest/gtest.h>

import parking.futex;
import parking.pthread;

TEST(Parking, BasicTest) {
    {
        auto start = std::chrono::steady_clock::now();
        parking::futex::Parker p {};
        p.park_timeout(std::chrono::milliseconds(200));
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(duration.count(), 200) << "Futex parker timeout too short: " << duration.count() << "ms";
        EXPECT_LT(duration.count(), 210) << "Futex parker timeout too long: " << duration.count() << "ms";
    }

    {
        auto start = std::chrono::steady_clock::now();
        parking::pthread::Parker p {};
        p.park_timeout(std::chrono::milliseconds(200));
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(duration.count(), 200) << "Pthread parker timeout too short: " << duration.count() << "ms";
        EXPECT_LT(duration.count(), 210) << "Pthread parker timeout too long: " << duration.count() << "ms";
    }
}
