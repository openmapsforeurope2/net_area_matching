// APP
#include <app/calcul/SplitAreaMergerOp.h>
#include <app/params/ThemeParameters.h>
// #include <app/tools/zTools.h>
// #include <app/tools/geometry/PolygonSplitter.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
// #include <ign/math/Line2T.h>
// #include <ign/geometry/io/WkbReader.h>
// #include <ign/geometry/algorithm/StraightSkeletonCGAL.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
// #include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>
// #include <epg/tools/geometry/project.h>
// #include <epg/tools/geometry/interpolate.h>
// #include <epg/tools/geometry/LineIntersector.h>
// #include <epg/tools/geometry/SegmentIndexedGeometry.h>
// #include <epg/tools/geometry/getLength.h>

// #include <ome2/geometry/tools/lineStringTools.h>
// #include <ome2/geometry/tools/GetEndingPointsOp.h>
// #include <ome2/geometry/tools/isSlimSurface.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        SplitAreaMergerOp::SplitAreaMergerOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        SplitAreaMergerOp::~SplitAreaMergerOp()
        {
            _shapeLogger->closeShape("msa_small_merged");
            _shapeLogger->closeShape("msa_same_id_merged");
        }

        ///
        ///
        ///
        void SplitAreaMergerOp::compute(
			bool verbose
		) {
            SplitAreaMergerOp splitAreaMergerOp(verbose);
            splitAreaMergerOp._compute();
        }


        ///
        ///
        ///
        void SplitAreaMergerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("msa_small_merged", epg::log::ShapeLogger::POLYGON);
            _shapeLogger->addShape("msa_same_id_merged", epg::log::ShapeLogger::POLYGON);

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
        };

        ///
        ///
        ///
        void SplitAreaMergerOp::_compute() const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
            double const areaThreshold = themeParameters->getValue(SAM_SMALL_AREA_THRESHOLD).toDouble();
            std::string const nationalIdName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();

            // paquets de areas a merger
            std::vector<std::map<std::string, ign::feature::Feature>> vmAreas;

            std::map<double, ign::feature::Feature> mSortedSmallAreas;
            std::set<std::string> sSmallAreas;

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string areaId = fArea.getId();

                double area = mp.area();

                if( area > areaThreshold) continue;

                mSortedSmallAreas.insert(std::make_pair(area, fArea));
                sSmallAreas.insert(fArea.getId());
            }

            for ( std::map<double, ign::feature::Feature>::const_reverse_iterator rmit = mSortedSmallAreas.rbegin() ; rmit != mSortedSmallAreas.rend() ; ++rmit ) {
                std::map<double, ign::feature::Feature> mNeighbours = _getNeighboursWithArea(rmit->second);
                if (sSmallAreas.find(rmit->second.getId()) != sSmallAreas.end()) continue;

                _addAreas(rmit->second, mNeighbours.rbegin()->second, vmAreas);
            }

            ign::feature::FeatureIteratorPtr itArea2 = _fsArea->getFeatures(filterArea);
            while (itArea2->hasNext())
            {
                ign::feature::Feature const& fArea = itArea2->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string areaId = fArea.getId();
                std::string identifier = fArea.getAttribute(nationalIdName).toString();
                
                if ( sSmallAreas.find(areaId) != sSmallAreas.end() ) continue;

                std::vector<ign::feature::Feature> vNeighbours = _getNeighbours(fArea);

                for (std::vector<ign::feature::Feature>::const_iterator vit = vNeighbours.begin() ; vit != vNeighbours.end() ; ++vit) {
                    std::string identifierNeighbour = vit->getAttribute(nationalIdName).toString();
                    if( identifierNeighbour == identifier ) {
                        _addAreas(*vit, fArea, vmAreas);
                    }
                }
            }

            _mergeGroups(vmAreas);

            for (std::vector<std::map<std::string, ign::feature::Feature>>::iterator vit = vmAreas.begin() ; vit != vmAreas.end() ; ++vit) {
                double maxArea = 0;
                ign::feature::Feature* maxFeatPtr = 0;
                ign::geometry::GeometryPtr resultingGeomPtr(new ign::geometry::MultiPolygon());
                for(std::map<std::string, ign::feature::Feature>::iterator mit = vit->begin() ; mit != vit->end() ; ++mit) {
                    ign::geometry::MultiPolygon const& mp = mit->second.getGeometry().asMultiPolygon();
                    resultingGeomPtr.reset(resultingGeomPtr->Union(mp));
                    double area = mp.area();
                    if (area > maxArea) {
                        maxArea = area;
                        maxFeatPtr = &mit->second;
                    }
                    _fsArea->deleteFeature(mit->first);
                }

                maxFeatPtr->setGeometry(*resultingGeomPtr);
                _fsArea->createFeature(*maxFeatPtr);
            }

        }

        ///
        ///
        ///
        void SplitAreaMergerOp::_mergeGroups(std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas) const {
            for(size_t i = vmAreas.size()-1 ; i > 0 ; --i) {
                for( int j = i-1; j > -1 ; --j) {
                    bool found = false;
                    for( std::map<std::string, ign::feature::Feature>::const_iterator mit = vmAreas[i].begin() ; mit != vmAreas[i].end() ; ++mit) {
                        if (vmAreas[j].find(mit->first) != vmAreas[j].end()) {
                            vmAreas[j].insert(vmAreas[i].begin(), vmAreas[i].end());
                            vmAreas.pop_back();
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }
        }

        ///
        ///
        ///
        bool SplitAreaMergerOp::_addAreas(
            ign::feature::Feature const& feat1,
            ign::feature::Feature const& feat2,
            std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas
        ) const {
            bool foundGroup = false;
            for(std::vector<std::map<std::string, ign::feature::Feature>>::iterator vit = vmAreas.begin() ; vit != vmAreas.end() ; ++vit) {
                if( vit->find(feat1.getId()) != vit->end() ) {
                    vit->insert(std::make_pair(feat2.getId(), feat2));
                    foundGroup = true;
                    break;
                }
                if( vit->find(feat2.getId()) != vit->end() ) {
                    vit->insert(std::make_pair(feat1.getId(), feat1));
                    foundGroup = true;
                    break;
                }
            }
            if(!foundGroup) {
                vmAreas.push_back(std::map<std::string, ign::feature::Feature>());
                vmAreas.back().insert(std::make_pair(feat1.getId(), feat1));
                vmAreas.back().insert(std::make_pair(feat2.getId(), feat2));
            }
            return foundGroup;
        }

        ///
        ///
        ///
        std::map<double, ign::feature::Feature> SplitAreaMergerOp::_getNeighboursWithArea(ign::feature::Feature const& fArea) const {
            std::map<double, ign::feature::Feature> mNeighbours;

            ign::geometry::MultiPolygon const& areaGeom = fArea.getGeometry().asMultiPolygon();
            std::string areaId = fArea.getId();

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + areaGeom.toString() + "'),3035))");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            
            while (itArea->hasNext())
            {
                ign::feature::Feature const& fNeighbour = itArea->next();
                if(fNeighbour.getId() == areaId) continue;

                mNeighbours.insert(std::make_pair(fNeighbour.getGeometry().asMultiPolygon().area(), fNeighbour));
            }
            return mNeighbours;
        }

        ///
        ///
        ///
        std::vector<ign::feature::Feature> SplitAreaMergerOp::_getNeighbours(ign::feature::Feature const& fArea) const {
            std::vector<ign::feature::Feature> vNeighbours;

            ign::geometry::MultiPolygon const& areaGeom = fArea.getGeometry().asMultiPolygon();
            std::string areaId = fArea.getId();

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + areaGeom.toString() + "'),3035))");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            
            while (itArea->hasNext())
            {
                ign::feature::Feature const& fNeighbour = itArea->next();
                if(fNeighbour.getId() == areaId) continue;

                vNeighbours.push_back(fNeighbour);
            }
            return vNeighbours;
        }
    }
}
