#include "Processor.hpp"

Processor::Processor(FdTable & fdTable, Monitor & monitor)
                      : _fdTable(fdTable), _monitor(monitor) {}

Processor::~Processor(void) {}

std::string Processor::_getCgiInterpreter(ConnectionData & connData,
                            std::string const & extension) const
{
  std::map<std::string, std::string>::const_iterator  it;
  ServerConfig const *  serverConfig = connData.getServer();

  it = serverConfig->cgiInterpreter.find(extension);
  if (it == serverConfig->cgiInterpreter.end())
    return ("");
  return (it->second);
}
