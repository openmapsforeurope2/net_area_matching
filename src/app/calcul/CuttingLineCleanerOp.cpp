// APP
#include <app/calcul/CuttingLineCleanerOp.h>
#include <app/params/ThemeParameters.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        CuttingLineCleanerOp::CuttingLineCleanerOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        CuttingLineCleanerOp::~CuttingLineCleanerOp()
        {
            _shapeLogger->closeShape("ccf_deleted_cl");
        }

        ///
        ///
        ///
        void CuttingLineCleanerOp::compute(
			bool verbose
		) {
            CuttingLineCleanerOp CuttingLineCleanerOp(verbose);
            CuttingLineCleanerOp._compute();
        }


        ///
        ///
        ///
        void CuttingLineCleanerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("ccf_deleted_cl", epg::log::ShapeLogger::LINESTRING);
            
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            // app parameters
            params::ThemeParameters * themeParameters = params::ThemeParametersS::getInstance();
            std::string clTableName = themeParameters->getValue(CUTL_TABLE).toString();
            if (clTableName == "") {
                std::string const clTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
                clTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + clTableSuffix;
            }

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _fsCl = context->getDataBaseManager().getFeatureStore(clTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void CuttingLineCleanerOp::_compute() const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const linkedFeatIdName = epgParams.getValue(LINKED_FEATURE_ID).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            //--
            std::set<std::string> sCl2Delete;

            ign::feature::FeatureFilter filterCl;
            ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
            while (itCl->hasNext())
            {
                ign::feature::Feature const& fCl = itCl->next();
                std::string clId = fCl.getId();
                std::string linkedFeatId = fCl.getAttribute(linkedFeatIdName).toString();

                if (clId == "16702069-36f2-41c3-b0f1-bcf4a8e91a78") {
                    bool test = true;
                }

                std::vector<std::string> vIds;
                epg::tools::StringTools::Split(linkedFeatId, "#", vIds);

                bool bDelete = true;
                for (size_t i = 0 ; i < vIds.size() ; ++i) {
                    ign::feature::Feature fArea;
                    _fsArea->getFeatureById(vIds[i], fArea);
                    if(fArea.getId() != "") {
                        bDelete = false;
                        break;
                    }
                }

                if ( !bDelete ) continue;

                // les surfaces d'origine de la CL peuvent avoir été coupées
                // au step de cleanByLandmask et avoir changer d'id
                // on vérifie donc que la CL n'est en contact avec aucune surface
                // et qu'elle doit donc bien être supprimée

                ign::geometry::LineString const& clGeom = fCl.getGeometry().asLineString();
                std::string country = fCl.getAttribute(countryCodeName).toString();

                if( _hasIntersectingAreas(clGeom, country) ) continue;

                sCl2Delete.insert(clId);

                _shapeLogger->writeFeature("ccf_deleted_cl", fCl);
            }

            for ( std::set<std::string>::const_iterator sit = sCl2Delete.begin() ; sit != sCl2Delete.end() ; ++sit) {
                _fsCl->deleteFeature(*sit);
            }
        };

        ///
        ///
        ///
        bool CuttingLineCleanerOp::_hasIntersectingAreas( 
            ign::geometry::LineString const& clGeom,
            std::string const& country
        ) const {

            // context
            epg::Context* context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            std::string sql = "SELECT count(*) FROM "+ areaTableName;
            sql += " WHERE "+countryCodeName+"='"+country+"'";
            sql += " AND ST_DISTANCE(" + geomName + ", ST_GeomFromText('" + clGeom.toString() + "')) < 0.001";

            //--
            ign::sql::SqlResultSetPtr	resultPtr = context->getDataBaseManager().getConnection()->query( sql );

            return resultPtr->getFieldValue(0,0).toInteger() > 0;
        }
    }
}
