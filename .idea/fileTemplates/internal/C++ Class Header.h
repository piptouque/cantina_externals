#parse("C File Header.h")
#[[#ifndef]]# ${INCLUDE_GUARD}
#[[#define]]# ${INCLUDE_GUARD}

#[[#include]]# <cant/common/macro.hpp>
${NAMESPACES_OPEN}

class ${NAME} 
{

};

${NAMESPACES_CLOSE}
#[[#include]]# <cant/common/undef_macro.hpp>

#[[#endif]]# //${INCLUDE_GUARD}
