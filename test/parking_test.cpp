#include <gtest/gtest.h>
#include <memory>
#include <thread>

import parking.futex;
import parking.pthread;

TEST(Parking, BasicTest) {
    {
        auto                   start = std::chrono::steady_clock::now();
        parking::futex::Parker p {};
        p.park_timeout(std::chrono::milliseconds(200));
        auto end      = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(duration.count(), 200)
            << "Futex parker timeout too short: " << duration.count() << "ms";
        EXPECT_LT(duration.count(), 210)
            << "Futex parker timeout too long: " << duration.count() << "ms";
    }

    {
        auto                     start = std::chrono::steady_clock::now();
        parking::pthread::Parker p {};
        p.park_timeout(std::chrono::milliseconds(200));
        auto end      = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(duration.count(), 200)
            << "Pthread parker timeout too short: " << duration.count() << "ms";
        EXPECT_LT(duration.count(), 210)
            << "Pthread parker timeout too long: " << duration.count() << "ms";
    }
}

TEST(ParkingThread, BasicTest) {
    auto        p = std::make_shared<parking::futex::Parker>();
    std::thread t([p, start = std::chrono::steady_clock::now()] {
        std::cout << "thread parking" << std::endl;
        p->park();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        EXPECT_GE(duration.count(), 1000);
        EXPECT_LT(duration.count(), 1050);
        std::cout << "thread unpark" << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    p->unpark();
    if (t.joinable()) t.join();
}