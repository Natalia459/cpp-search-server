#pragma once
#include <string>
#include "log_duration.h"
#include "search_server.h"
#include "document.h"

void AddDocument(SearchServer&, int, const std::string&, DocumentStatus, const std::vector<int>&);

void MatchDocuments(SearchServer&, const std::string&);

void FindTopDocuments(SearchServer&, const std::string&);

std::ostream& operator<<(std::ostream&, std::tuple<std::vector<std::string>, DocumentStatus>);