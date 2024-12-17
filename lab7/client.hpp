#include "utils.hpp"

std::mutex mtx;
std::condition_variable response;
std::queue<packet> msgQueue;
