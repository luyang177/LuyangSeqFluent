#include "LuyangSeq.h"
#include <iostream>

using namespace LuyangSeq;

int main()
{
	std::vector<int> vec{ 1,2,3,4,5,6,7,8,9 };
	auto iter = Seq(vec)
		>> Seq::Filter<int>([](int v) { return v % 2 == 0; })
		>> Seq::Take<int>(4)
		>> Seq::Map<int, std::string>([](int v) { return std::to_string(v + 100); })
		>> Seq::FlatMap<std::string, int>([](const std::string& v)
	{
		std::vector<int> result;
		for (auto item : v)
		{
			result.push_back((int)item - 48);
		}
		return Seq(result);
	});


	while (iter->HasNext())
	{
		auto next = iter->Next();
		std::cout << next << std::endl;
	}

	int n;
	std::cin >> n;
}