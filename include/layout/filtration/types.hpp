#include "graph/types.hpp"
#include <cstdint>
#include <vector>

typedef struct {
	uint32_t m_layerCount;
	std::vector<uint32_t> m_borders;
	std::vector<NodeID> m_filtration;
} NestedFiltrationResult;
