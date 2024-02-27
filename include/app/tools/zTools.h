#ifndef _APP_TOOLS_ZTOOLS_H_
#define _APP_TOOLS_ZTOOLS_H_

// SOCLE
#include <ign/geometry/Geometry.h>

namespace app{
namespace tools{

    /// @brief 
    /// @param geom 
    void zFiller( ign::geometry::Geometry & geom, double value );

    /// @brief 
    /// @param geom 
    void removePointWithZ( ign::geometry::Geometry & geom, double value );

}
}

#endif