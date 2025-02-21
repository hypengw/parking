# parking
Thread parking and unparking module for c++ 20.   

Port from rust std;

## Impl
| system | api |
| -- | -- |
| linux | futex |
| windows | WaitOnAddress |


## Examples

```c++
import parking;

#include <thread>
#include <memory>

auto p = std::make_shared<parking::Parker>();

std::thread t([p] {
    std::cout << "thread parking" << std::endl;
    p->park();
    std::cout << "thread unpark" << std::endl;
});

std::this_thread::sleep_for(std::chrono::milliseconds(1000));
p.unpark();
if (t.joinable()) t.join();
```