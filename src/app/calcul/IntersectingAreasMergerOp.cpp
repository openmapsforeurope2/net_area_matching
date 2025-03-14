// APP
#include <app/calcul/IntersectingAreasMergerOp.h>
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
        IntersectingAreasMergerOp::IntersectingAreasMergerOp(
            std::string const& borderCode,
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
        IntersectingAreasMergerOp::~IntersectingAreasMergerOp()
        {
            _shapeLogger->closeShape("iam_merged_area");
        }

        ///
        ///
        ///
        void IntersectingAreasMergerOp::Compute(
            std::string const& borderCode,
			bool verbose
		) {
            IntersectingAreasMergerOp intersectingAreasMergerOp(borderCode, verbose);
            intersectingAreasMergerOp._compute();
        }

        ///
        ///
        ///
        void IntersectingAreasMergerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("iam_merged_area", epg::log::ShapeLogger::POLYGON);

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
        void IntersectingAreasMergerOp::_compute() const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();

            size_t idCountryRef = 0;

            ign::feature::FeatureFilter filterArea(countryCodeName+"='"+_vCountry[idCountryRef]+"'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ intersecting areas merger (1/2) % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

            std::list<std::set<std::string>> lsAreas2Merge;
            std::set<std::string> sTreatedArea;
            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();

                if (sTreatedArea.find(idOrigin) != sTreatedArea.end()) continue;

                std::set<std::string> sAreas2Merge;
                sAreas2Merge.insert(idOrigin);

                _getIntersectingAreas(mp, idCountryRef, sAreas2Merge);

                sTreatedArea.insert(sAreas2Merge.begin(), sAreas2Merge.end());

                if (sAreas2Merge.size() > 1) 
                    lsAreas2Merge.push_back(sAreas2Merge);
            }

            boost::progress_display display2(lsAreas2Merge.size(), std::cout, "[ intersecting areas merger (2/2) % complete ]\n");

            for (std::list<std::set<std::string>>::const_iterator lit = lsAreas2Merge.begin() ; lit != lsAreas2Merge.end() ; ++lit) {
                ign::feature::FeatureFilter filterArea(idName + " IN " +_toSqlList(*lit));
                ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

                ign::geometry::GeometryPtr mergedGeomPtr(new ign::geometry::MultiPolygon());
                while (itArea->hasNext())
                {
                    ign::feature::Feature const& fArea = itArea->next();
                    ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();

                    mergedGeomPtr.reset(mergedGeomPtr->Union(mp));
                }

                ign::feature::Feature mergedFeat = _fsArea->newFeature();
                mergedFeat.setAttribute(countryCodeName, ign::data::String(_borderCode));
                if (mergedGeomPtr->isMultiPolygon()) {
                    ign::geometry::MultiPolygon const& mpResult = mergedGeomPtr->asMultiPolygon();

                    for ( size_t i = 0 ; i < mpResult.numGeometries() ; ++i) {
                        mergedFeat.setGeometry(mpResult.polygonN(i).toMulti());
                        _fsArea->createFeature(mergedFeat);
                    }
                } else {
                    mergedFeat.setGeometry(mergedGeomPtr->asPolygon().toMulti());
                    _fsArea->createFeature(mergedFeat);
                }

                for (std::set<std::string>::const_iterator sit = lit->begin(); sit != lit->end() ; ++sit)
                    _fsArea->deleteFeature(*sit);
            }
        }


        ///
        ///
        ///
        void IntersectingAreasMergerOp::_getIntersectingAreas(
            ign::geometry::MultiPolygon const& mp_, 
            size_t country, 
            std::set<std::string> & sIntersectingArea
        ) const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const idName = epgParams.getValue(ID).toString();

            std::list<std::pair<size_t, ign::geometry::MultiPolygon>> stack(1, std::make_pair(country, mp_));
            do  {
                ign::feature::FeatureFilter filterArea(countryCodeName+"='"+_vCountry[1-stack.front().first]+"'");
                epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + stack.front().second.toString() + "'),3035))");
                epg::tools::FilterTools::addAndConditions(filterArea, idName + " NOT IN " +_toSqlList(sIntersectingArea));

                ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

                while (itArea->hasNext())
                {
                    ign::feature::Feature const& fArea = itArea->next();
                    ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                    std::string idOrigin = fArea.getId();

                    sIntersectingArea.insert(idOrigin);
                    stack.push_back(std::make_pair(1-stack.front().first, mp));
                }
                stack.pop_front();

            } while ( stack.size() > 0 );
            
        }


        ///
        ///
        ///
        std::string IntersectingAreasMergerOp::_toSqlList(std::set<std::string> const& s, std::string separator) const {
            std::string result = "";
            for (std::set<std::string>::const_iterator sit = s.begin() ; sit != s.end() ; ++sit) {
                if (sit != s.begin()) result += ",";
                result += "'"+*sit+"'";
            }
            return "("+result+")";
        }
    }
}