// APP
#include <app/utils/createAllTables.h>
#include <app/params/ThemeParameters.h>

// EPG
#include <epg/Context.h>


namespace app{
namespace utils{

    //--
    void createClTable() {
        epg::Context* context = epg::ContextS::getInstance();
        app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();

        std::string const idName = context->getEpgParameters().getValue(ID).toString();
	    std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
        std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
        std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

        std::string const cutlTableName = themeParameters->getValue(CUTL_TABLE).toString();
        if (!context->getDataBaseManager().tableExists(cutlTableName)) {
            std::ostringstream ss;
            ss << "DROP TABLE IF EXISTS " << cutlTableName << " ;";
            ss << "CREATE TABLE " << cutlTableName <<"("
                << idName << " uuid DEFAULT gen_random_uuid(),"
                << geomName << " geometry(LINESTRINGZ),"
                << countryCodeName << " character varying(8),"
                << linkedFeatIdName << " character varying(255) "
                << ");";

            context->getDataBaseManager().getConnection()->update(ss.str());
        }

        std::string const intAreaTableName = themeParameters->getValue(INTERSECTION_AREA_TABLE).toString();
        if (!context->getDataBaseManager().tableExists(intAreaTableName)) {
            std::ostringstream ss;
            ss << "DROP TABLE IF EXISTS " << intAreaTableName << " ;";
            ss << "CREATE TABLE " << intAreaTableName << "("
                << idName << " uuid DEFAULT gen_random_uuid(),"
                << geomName << " geometry(MULTIPOLYGONZ),"
                << countryCodeName << " character varying(8),"
                << linkedFeatIdName << " character varying(255) "
                << ");";

            context->getDataBaseManager().getConnection()->update(ss.str());
        }

        std::string const cutpTableName = themeParameters->getValue(CUTP_TABLE).toString();
        std::string const sectionGeomName = themeParameters->getValue(CUTP_SECTION_GEOM).toString();
        if (!context->getDataBaseManager().tableExists(cutpTableName)) {
            std::ostringstream ss;
            ss << "DROP TABLE IF EXISTS " << cutpTableName << " ;";
            ss << "CREATE TABLE " << cutpTableName << "("
                << idName << " uuid DEFAULT gen_random_uuid(),"
                << geomName << " geometry(PointZ),"
                << sectionGeomName << " geometry(LINESTRING),"
                << countryCodeName << " character varying(8),"
                << linkedFeatIdName << " character varying(255) "
                << ");";

            context->getDataBaseManager().getConnection()->update(ss.str());
        }
    }
}
}