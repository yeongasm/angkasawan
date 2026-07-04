#include "fmt/printf.h"
#include "warp/warp.hpp"

auto foo() -> warp::task<int>
{
	co_return 1;
}

auto bar() -> warp::task<void>
{
	auto f = foo();
	auto result = co_await f;
	fmt::println("result: {}", result);
}

auto main() -> int
{
	auto b = bar();
	warp::sync_wait(b);
	return 0;
}