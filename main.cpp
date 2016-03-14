/*
 * Copyright (c) 2016
 *  Somebody
 */

char const copyright[] =
"Copyright (c) 2016\n\tSomebody.  All rights reserved.\n\n";

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char const *argv[])
{
	std::vector<std::string> vecArgs(argv, argv + argc);

    std::cout << copyright << std::endl;
	std::copy(vecArgs.begin(), vecArgs.end(), std::ostream_iterator<std::string>(std::cout, ", "));

	return 0;
}

