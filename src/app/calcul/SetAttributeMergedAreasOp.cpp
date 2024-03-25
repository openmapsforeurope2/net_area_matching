// APP
#include <app/calcul/SetAttributeMergedAreasOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        SetAttributeMergedAreasOp::SetAttributeMergedAreasOp(
            std::string borderCode,
            bool verbose
        ) : 
            _verbose(verbose),
            _borderCode(borderCode)
        {
            _init();
        }

        ///
        ///
        ///
        SetAttributeMergedAreasOp::~SetAttributeMergedAreasOp()
        {
        }

        ///
        ///
        ///
        void SetAttributeMergedAreasOp::compute(
            std::string borderCode,
			bool verbose
		) {
            SetAttributeMergedAreasOp setAttributeMergedAreasOp(borderCode, verbose);
			setAttributeMergedAreasOp._compute();
        }

        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());

            //--
		    epg::tools::StringTools::Split(_borderCode, "#", _vCountry);
        }


        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_compute() const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName+"='%#%'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ set attribute merged areas % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

            std::list<std::set<std::string>> lsAreas2Merge;
            std::set<std::string> sTreatedArea;
            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();
				ign::geometry::Polygon geomArea = fArea.getGeometry().asPolygon();

            }

        }


    }
}