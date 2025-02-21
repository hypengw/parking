export module parking;

#if defined(__linux__) || defined(_WIN32)
export import parking.futex;
#else
export import parking.pthread;
#endif

namespace parking
{
#if defined(__linux__) || defined(_WIN32)
export using Parker = futex::Parker;
#else
export using Parker = pthread::Parker;
#endif

} // namespace parking