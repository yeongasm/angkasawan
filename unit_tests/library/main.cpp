#include <gtest/gtest.h>

// Include foundation headers.
#include "lib/array.h"
#include "lib/string.h"
#include "lib/map.h"
#include "lib/set.h"

/**
* UNIT TEST(Array)
* 
* A. Insert & delete.
*	1. Push 100 000 elements into the container.
*	2. Pop 100 000 elements from the container.
* 
* B. Reserve, insert & delete.
*	1. Reserve space for 100 000 elements.
*	2. Push 100 000 elements into the container.
*	3. Pop 100 000 elements from the container.
* 
* C. Reserve 100 000, insert 100 000, insert another 100 000, manually release
*	1. Reserve space for 100 000 elements.
*	2. Push 100 000 elements into the container.
*	3. Push another 100 000 elements into the container.
*	4. Manually release memory in the array.
* 
* D. Insert, pop in the middle.
*	1. Insert 100 elements.
*	2. Pop 10 elements from the middle, starting at index 45.
*	3. Iterate.
*	4. Check if result is expected.
*/

TEST(ArrayFoundationUnitTest, InsertDelete) 
{
	constexpr size_t count = 100000;
	lib::array<int32> testArray;
	for (size_t i = 0; i < count; i++)
	{
		testArray.push_back(static_cast<int32>(i));
	}
	EXPECT_EQ(testArray.size(), count);

	testArray.pop(count);

	EXPECT_EQ(testArray.size(), 0);
}

TEST(ArrayFoundationUnitTest, ReserveInsertDelete)
{
	constexpr size_t count = 100000;
	lib::array<int32> testArray;
	testArray.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		testArray.push_back(static_cast<int32>(i));
	}
	EXPECT_EQ(testArray.size(), count);

	testArray.pop(count);

	EXPECT_EQ(testArray.size(), 0);
}

TEST(ArrayFoundationUnitTest, ReserveInsertInsertRelease)
{
	constexpr size_t count = 100000;
	lib::array<int32> testArray;
	testArray.reserve(count);
	EXPECT_EQ(testArray.capacity(), count);
	for (size_t i = 0; i < count; i++)
	{
		testArray.push_back(static_cast<int32>(i));
	}
	EXPECT_EQ(testArray.size(), count);

	for (size_t i = 0; i < count; i++)
	{
		testArray.push_back(static_cast<int32>(i));
	}
	EXPECT_EQ(testArray.size(), count * 2);

	testArray.release();

	EXPECT_EQ(testArray.size(), 0);
	EXPECT_EQ(testArray.data(), nullptr);
}

TEST(ArrayFoundationUnitTest, InsertPopInMiddle)
{
	lib::array<int32> controlArray;

	for (size_t i = 0; i < 90; i++)
	{
		if (i >= 45)
		{
			controlArray.push_back(static_cast<int32>(i + 10));
			continue;
		}
		controlArray.push_back(static_cast<int32>(i));
	}

	constexpr size_t count = 100;
	lib::array<int32> testArray;
	for (size_t i = 0; i < count; i++)
	{
		testArray.push_back(static_cast<int32>(i));
	}
	EXPECT_EQ(testArray.size(), count);

	for (size_t i = 0; i < 10; i++)
	{
		testArray.pop_at(45);
	}
	EXPECT_EQ(testArray.size(), 90);
	bool isDifferent = false;
	for (size_t i = 0; i < 90; i++)
	{
		if (testArray[i] != controlArray[i])
		{
			isDifferent = true;
			break;
		}
	}
	EXPECT_EQ(isDifferent, false);
}

/**
* UNIT TEST(String)
*
* A. Assigning a small string.
*	1. Assign "Hello World!" to string.
*	2. Check length, should be less than 24.
*	3. is_small_string() should return true.
*
* B. Assigning a small string and then into a large string.
*	1. Assign "Hello World!" to string.
*	2. Check length, should be less than 24.
*	3. is_small_string() should return true.
*	4. Assign "The quick brown fox jumps over the lazy dog!" to string.
*	5. Check length, should be more than 24.
*	6. is_small_string() should return false.
* 
* C. Small string -> large string -> small string
*	1. Assign "Hello World!" to string.
*	2. Check length, should be less than 24.
*	3. is_small_string() should return true.
*	4. Assign "The quick brown fox jumps over the lazy dog!" to string.
*	5. Check length, should be more than 24.
*	6. is_small_string() should return false.
*	7. Assign "Hello World!" to string.
*	8. Check length, should be less than 24.
*	9. is_small_string() should return false.
* 
* D. Formating.
*	1. Assign "Hello World! 123" to string via .format().
*	2. Check length, should be less than 24.
*	3. is_small_string() should return true.
*	4. Assign "The quick brown fox jumps over the lazy dog. See you soon space cowboy" to string via global format().
*	5. Check length, should be more than 24.
*	6. is_small_string() should return false.
* 
* E. Substr
*	1. Assign "Hello World!" to string A.
*	2. Assign A.substr(0, 5) to string B.
*	3. string A should be equals to "Hello World!".
*	4. string B should be equals to "Hello".
* 
* F. Assign large string.
*	1. Assign "The quick brown fox jumps over the lazy dog to string.
* 
* G. Push each character into string.
*	1. For each character in "The quick brown fox jumps over the lazy dog!", push it into the string.
*	2. String should be equal to "The quick brown fox jumps over the lazy dog!".
*/

TEST(StringFoundationUnitTest, AssignSmallString)
{
	lib::string str = "Hello World!";
	EXPECT_LT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), true);
}

TEST(StringFoundationUnitTest, AssignSmallStringThenLarge)
{
	lib::string str = "Hello World!";
	EXPECT_LT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), true);
	str = "The quick brown fox jumps over the lazy dog!";
	EXPECT_GT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), false);
}

TEST(StringFoundationUnitTest, AssignSmallThenLargeThenSmall)
{
	lib::string str = "Hello World!";
	EXPECT_LT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), true);
	str = "The quick brown fox jumps over the lazy dog!";
	EXPECT_GT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), false);
	str = "Hello World!";
	EXPECT_LT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), false);
}

TEST(StringFoundationUnitTest, StringFormating)
{
	lib::string str;
	str.format("Hello World! {}", 123);
	EXPECT_LT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), true);
	str = lib::format("The quick brown fox jumps over the lazy dog! {}", "See you soon space cowboy.");
	EXPECT_GT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), false);
}

TEST(StringFoundationUnitTest, Substring)
{
	lib::string a = "Hello World!";
	lib::string b = a.substr(0, 5);
	EXPECT_EQ(a, "Hello World!");
	EXPECT_EQ(b, "Hello");
}

TEST(StringFoundationUnitTest, AssignLargeStringOnInit)
{
	lib::string str = "The quick brown fox jumps over the lazy dog!";
	EXPECT_GT(str.length(), 24);
	EXPECT_EQ(str.is_small_string(), false);
}

TEST(StringFoundationUnitTest, PushCharactersIntoString)
{
	const lib::string text = "The quick brown fox jumps over the lazy dog!";
	lib::string str{ text.length() + 1 };

	for (const char ch : text)
	{
		str.push(ch);
	}
	EXPECT_EQ(str, text);
}

/**
* UNIT TEST(Map & Set [Hash Container])
*
* A. Assigning string as keys and integers as values.
*	1. Emplace a pair with key = "one" and value = 1 into the map.
*	2. Emplace a pair with key = "two" and value = 2 into the map.
*	3. Emplace a pair with key = "three" and value = 3 into the map.
*	4. Get value for key = "one".
*	5. Get value for non-existent key = "four" and assign the value 4.
*	6. Length of map should be 4.
*	7. Value of key = "four" should be equals to 4.
* 
* B. Emplace unique pointer into map.
*	1. Emplace a pair with key = "first_element" and value = UniquePtr(5) into the map.
*	2. Get the reference to the value for key = "first_element".
*	3. UniquePtr of key = "first_element" should hold the value = 5.
*	4. Length of map should be 1.
* 
* C. Reserve space for 4 elements and insert 6.
*	1. Reserve map's capacity for 4 elements.
*	2. Loop through an array of std::pair<lib::string, int32> of size 6, and emplace each element.
*	3. Observe length of map should be 6.
*	4. Observe capacity of map should be 20.
*	5. Iterate objects in the map and count sum of all value elements.
*	6. Sum of all value elements should be correct.
* 
* D. Reserve space for 4 element, insert 6, shrink to 4.
*/

TEST(HashContainerUnitTest, MapAssignStringAsKeyAndIntAsValue)
{
	lib::map<lib::string, int32> foo;
	foo.emplace(lib::string{ "one" },	1);
	foo.emplace(lib::string{ "two" },	2);
	foo.emplace(lib::string{ "three" }, 3);

	int32& value = foo["one"];
	EXPECT_EQ(value, 1);

	foo["four"] = 4;
	EXPECT_EQ(foo["four"], 4);
	EXPECT_EQ(foo.size(), 4);
}

TEST(HashContainerUnitTest, MapEmplaceUniquePointers)
{
	lib::map<lib::string, lib::unique_ptr<int32>> foo;
	foo.emplace("first_element", lib::make_unique<int32>(5));

	lib::unique_ptr<int32>& pointer = foo["first_element"];
	EXPECT_EQ(*pointer, 5);
	EXPECT_EQ(foo.size(), 1);
}

TEST(HashContainerUnitTest, HashContainerTestRehash)
{
	int32 sum = 0;	// 21.
	const lib::array<std::pair<lib::string, int32>> arr = {
		{ "one",	 1 },
		{ "two",	 2 },
		{ "three",	 3 },
		{ "four",	 4 },
		{ "five",	 5 },
		{ "six",	 6 },
	};
	lib::map<lib::string, int32> foo;
	foo.reserve(4);

	for (auto& pair : arr)
	{
		foo.emplace(pair);
	}

	EXPECT_EQ(foo.size(), 6);
	EXPECT_EQ(foo.capacity(), 20);

	for (const auto& [key, value] : foo)
	{
		sum += value;
	}
	EXPECT_EQ(sum, 21);
}

TEST(HashContainerUnitTest, CopyHashContainer)
{
	lib::map<lib::string, int32> a;
	a.emplace(lib::string{ "one" }, 1);
	a.emplace(lib::string{ "two" }, 2);
	a.emplace(lib::string{ "three" }, 3);

	lib::map<lib::string, int32> b = a;
	EXPECT_EQ(b.size(), a.size());
	EXPECT_EQ(b["one"], 1);
	EXPECT_EQ(b["two"], 2);
	EXPECT_EQ(b["three"], 3);
	EXPECT_EQ(b.size(), a.size());
}

TEST(HashContainerUnitTest, HashContainerTestRehashAndShrink)
{
	const lib::array<std::pair<lib::string, int32>> arr = {
		{ "one",	 1 },
		{ "two",	 2 },
		{ "three",	 3 },
		{ "four",	 4 },
		{ "five",	 5 },
		{ "six",	 6 },
	};
	lib::map<lib::string, int32> foo;
	foo.reserve(4);

	for (auto& pair : arr)
	{
		foo.emplace(pair);
	}

	EXPECT_EQ(foo.size(), 6);
	EXPECT_EQ(foo.capacity(), 20);

	foo.reserve(4);
	EXPECT_EQ(foo.capacity(), 4);
}