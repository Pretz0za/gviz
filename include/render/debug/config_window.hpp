#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "config/configuration.hpp"
#include <string>
#include <vector>

class ConfigWindow {
public:
  void Add(std::string label, IConfiguration &config);

  void Render(bool &open);

private:
  struct Entry {
    std::string label;
    IConfiguration *config;
  };

  std::vector<Entry> m_entries;
};

#endif
