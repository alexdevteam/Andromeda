#ifndef ANDROMEDA_DEBUG_QUAD_COMPONENT_HPP_
#define ANDROMEDA_DEBUG_QUAD_COMPONENT_HPP_

#include <andromeda/util/handle.hpp>

namespace andromeda {
class Mesh;
}

namespace andromeda::components {

// Tag component. Renders a quad (temporary)
struct [[component]] DbgQuad {
	Handle<Mesh> mesh;
};

}

#endif