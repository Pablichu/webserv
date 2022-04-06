#include "Config.hpp"

bool  generateConfigFile(std::ifstream & file,
          std::string const & sampleFilePath, bool & expectValid)
{
  std::ofstream configFile;
  std::string   line;

  configFile.open(sampleFilePath.c_str());
  if (!configFile.is_open())
    return (false);
  std::getline(file, line);
  if (line.find("invalid") != std::string::npos)
    expectValid = false;
  else
    expectValid = true;
  while (std::getline(file, line))
  {
    if (line == "###")
      break ;
    configFile << line;
  }
  configFile.close();
  if (line != "###")
    return (false);
  return (true);
}

void  testConfigs(std::ifstream & file, Config & config,
                    std::string const & sampleFilePath)
{
  bool        next;
  bool        expectValid;
  bool        valid;
  std::size_t i;

  std::cout << "\n";
  i = 1;
  while (1)
  {
    next = generateConfigFile(file, sampleFilePath, expectValid);
    valid = config.parseFile();
    if (valid)
      std::cout << "#" << i << " Sample config is valid";
    else
      std::cout << "#" << i << " Sample config is invalid";
    if (valid != expectValid)
      std::cout << ": WRONG! Unexpected validity\n" << std::endl;
    else
      std::cout << ": OK\n" << std::endl;
    if (!next)
      break ;
    ++i;
  }
}

int main(int argc, char** argv)
{
  std::ifstream testFile;
  Config        config;

  if (argc != 3)
  {
    std::cout << "Need config test file";
    std::cout << " and empty sample test file paths" << std::endl;
  }
  testFile.open(argv[1]);
  if (!testFile.is_open())
  {
    std::cout << "No config test file available" << std::endl;
    return (1);
  }
  config.setPath(argv[2]);
  testConfigs(testFile, config, argv[2]);
  return (0);
}
