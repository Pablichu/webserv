#include "Processor.hpp"

Processor::Processor(FdTable & fdTable, Monitor & monitor)
                      : _fdTable(fdTable), _monitor(monitor) {}

Processor::~Processor(void) {}
