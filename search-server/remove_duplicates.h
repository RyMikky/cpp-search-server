#pragma once
#include "search_server.h"
#include <map>
#include <set>

void RemoveDuplicates(SearchServer& search_server);

void RemoveDuplicatesTwo(SearchServer& search_server);

std::set<std::string> WordsSetCreator(const std::map<std::string, double>& element);

bool WordsEqualCheck(const std::map<std::string, double>& first, const std::map<std::string, double>& second);