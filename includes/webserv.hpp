#pragma once

//Internal libraries
#include <iostream>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>

//Server
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "CgiHandler.hpp"
#include "FdTable.hpp"
#include "Monitor.hpp"
#include "Data.hpp"
#include "UrlParser.hpp"
