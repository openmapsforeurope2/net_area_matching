#ifndef _APP_UTILS_COPYTABLEUTILS_H_
#define _APP_UTILS_COPYTABLEUTILS_H_

#include <string>

#include <ign/geometry/Geometry.h>

namespace app{
namespace utils{

	/**
	* Utilitary class which provide functions to copy sql table
	*/
	class CopyTableUtils{
	public:

		/**
		* Copy a table of areas.
		* Code column is created if it doesn't exist in the source table.
		* Indexes are created on geometry and id fields.
		* @param refTableName name of the vertices table to copy
		* @param condition sql where clause to copy a restricted number of objects
		* @param askForConfirmation ask for confirmation before destroying the edge table
		* @param typeIsMultipolygon true if the type of geometry is multipolygon, false if polygon
		* @param withData if false onely the table structure is copied (without data).
		*/
		static bool copyAreaTable( 
			std::string const& refTableName,
			std::string const& condition = "",
			bool askForConfirmation = true,
			bool typeIsMultipolygon = true,
			bool withData = true
		);

	};

}
}

#endif