/**
 * @file
 */
#pragma once

#ifdef __FreeBSD__
#define _WITH_GETLINE /* For getline */
#include <sys/wait.h>  /* For WIF.. */
#endif

#include <vector>
#include <stack>
#include <list>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <regex>
#include <thread>
#include <iomanip>
#include <locale>
#include <codecvt>

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cassert>

#if __cplusplus > 201402L
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
namespace fs = boost::filesystem;
#endif

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"

#include <getopt.h>
#include <dlfcn.h>
#include <yaml.h>
#include <glob.h>
